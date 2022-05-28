#include "explorer.h"

#include <cstdio>
#include <iostream>
#include <thread>
#include <functional>
#include <algorithm>

namespace VSOMExplorer
{
    static bool showModelVectorsAsImage = false;
    static int modelVectorAsImageWidth = 28;
    static int modelVectorAsImageHeight = 28;
    
    int scaleColorToUCharRange(float value, float max, float min)
    {
        auto staticallyScaledValue = (value - min) * 255 / (max - min);

        return static_cast<int>(staticallyScaledValue > 255.f ? 255.f : staticallyScaledValue < 0.f ? 0.f : staticallyScaledValue);                
    }

    int scaleColorToUCharRangeWithZoom(float value, float max, float min, int outMax, int outMin)
    {
        auto scaledValue = scaleColorToUCharRange(value, max, min);

        auto span = outMax - outMin;
        auto dynamicallyScaledValue = (scaledValue - outMin) * 255.0 / span;
        auto contrainedValue = dynamicallyScaledValue < 0. ? 0. : dynamicallyScaledValue > 255.0 ? 255.0
                                                                                               : dynamicallyScaledValue;
        return static_cast<int>(contrainedValue);
    }

    static void RenderCombo(const char *name, const char **labels, const size_t numberOfChoices, size_t *currentId, const char *combo_preview_value)
    {
        if (ImGui::BeginCombo(name, combo_preview_value))
        {
            for (size_t n{0}; n < numberOfChoices; ++n)
            {
                const bool is_selected = (*currentId == n);
                if (ImGui::Selectable(labels[n], is_selected))
                    *currentId = n;

                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    static void RenderCombo(const std::string &name, const std::vector<std::string> &labels, size_t *currentId, const std::string &combo_preview_value)
    {
        auto c_labels = std::vector<char*>();
        c_labels.reserve(labels.size());

       std::transform(labels.begin(), labels.end(), std::back_inserter(c_labels), 
        [](const std::string & s)
            {
                char *pc = new char[s.size()+1];
                std::strcpy(pc, s.c_str());
                return pc; 
            }); 
        
        RenderCombo(name.c_str(), const_cast<const char**>(c_labels.data()), labels.size(), currentId, combo_preview_value.c_str());
    }

    static void LoadMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "CTRL+N"))
                {
                }
                if (ImGui::MenuItem("Quit", "CTRL+Q"))
                {
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "CTRL+Z"))
                {
                }
                if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
                {
                } // Disabled item
                ImGui::Separator();
                if (ImGui::MenuItem("Cut", "CTRL+X"))
                {
                }
                if (ImGui::MenuItem("Copy", "CTRL+C"))
                {
                }
                if (ImGui::MenuItem("Paste", "CTRL+V"))
                {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    static void DatasetEditor(DataSet &dataset)
    {
        if(ImGui::Begin("Dataset Editor"))
        {
            auto numberOfColumns = dataset.vectorLength();
            auto columnNames = dataset.getNames();
            auto columnWeights = dataset.getWeights(); // TODO: Create weight non-const accessor

            ImGui::Text("Columns:");

            for(size_t currentColumn{0}; currentColumn < numberOfColumns; ++currentColumn)
            {
                ImGui::InputFloat((columnNames[currentColumn] + " Weight").c_str(), &dataset.getWeight(currentColumn));
            }
        }
        ImGui::End();
    }

    static void DatasetViewer(const DataSet &dataset)
    {
        if (ImGui::Begin("Dataset"))
        {
            auto numberOfRows = dataset.size();
            numberOfRows = numberOfRows >= 100 ? 100 : numberOfRows;

            if (showModelVectorsAsImage)
            {
                float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
                auto p = ImGui::GetCursorScreenPos();
                auto original_p_x = p.x;
                const size_t width = modelVectorAsImageWidth, height = modelVectorAsImageWidth;
                const size_t x_offset = 1, y_offset = 1, step_size = 2;

                for (int n = 0; n < numberOfRows; ++n)
                {
                    ImDrawList *draw_list = ImGui::GetWindowDrawList();
                    auto currentNeuron = dataset.getData(n);

                    p.x += x_offset + width * step_size;

                    /* Draw actual image */
                    for (size_t i{0}; i < currentNeuron.size(); ++i)
                    {
                        const size_t x = i % width * step_size + x_offset;
                        const size_t y = i / height * step_size + y_offset;

                        auto currentValue = scaleColorToUCharRange(currentNeuron[i], 255.f, 0.f);

                        draw_list->AddRectFilled(ImVec2(p.x + x, p.y + y),
                                                ImVec2(p.x + x + step_size, p.y + y + step_size),
                                                IM_COL32(currentValue, currentValue, currentValue, 255));
                    }

                    /* Item list row wrap */
                    float last_image_x2 = p.x + (modelVectorAsImageWidth*step_size + x_offset);
                    float next_image_x2 = last_image_x2 + step_size*modelVectorAsImageWidth; // Expected position if next image was on same line
                    if (next_image_x2 > window_visible_x2)
                    {
                        p.y += modelVectorAsImageHeight*step_size + y_offset;
                        p.x = original_p_x;
                    }
                }
            }
            else
            {
                static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
                static bool display_headers = true;
                auto numberOfColumns = dataset.vectorLength();
                numberOfColumns = numberOfColumns >= 64 ? 64 : numberOfColumns;

                if (ImGui::BeginTable("table1", numberOfColumns, flags))
                {
                    if (display_headers)
                    {
                        for (size_t columnIndex{0}; columnIndex < numberOfColumns; ++columnIndex)
                            ImGui::TableSetupColumn(dataset.getName(columnIndex).c_str());
                        ImGui::TableHeadersRow();
                    }

                    for (size_t row{0}; row < numberOfRows; ++row)
                    {
                        auto rowValues = dataset.getData(row);
                        ImGui::TableNextRow();
                        for (size_t column{0}; column < numberOfColumns; ++column)
                        {
                            ImGui::TableSetColumnIndex(column);
                            auto columnValue = rowValues[column];
                            auto textValue = std::to_string(columnValue);
                            ImGui::TextUnformatted(textValue.c_str());
                        }
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    static void RenderUMatrix(const UMatrix &uMatrix)
    {
        if (ImGui::Begin("U-matrix"))
        {
            auto xSteps = uMatrix.getWidth();
            auto ySteps = uMatrix.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto maxValue = uMatrix.getMaxValue();

            ImDrawList *draw_list = ImGui::GetWindowDrawList();

            static float upper = 255.0f;
            static float lower = 0.0f;
            ImGui::DragFloat("Upper", &upper, 0.2f, 0.0f, 255.0f, "%.0f");
            ImGui::DragFloat("Lower", &lower, 0.2f, 0.0f, 255.0f, "%.0f");

            const ImVec2 p = ImGui::GetCursorScreenPos();

            for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
            {
                for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                {
                    auto unscaledValue = uMatrix.getValueAtIndex(xIndex, yIndex);
                    auto value = scaleColorToUCharRangeWithZoom(unscaledValue, maxValue, 0.f, static_cast<int>(upper), static_cast<int>(lower));
                    
                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
        }
        ImGui::End();
    }

    static void RenderWeigthMap(const Som &som)
    {
        if (ImGui::Begin("Weight Map"))
        {
            auto xSteps = som.getWidth();
            auto ySteps = som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto weightMap = som.getWeigthMap();
            auto maxValue = weightMap.maxCoeff();

            ImDrawList *draw_list = ImGui::GetWindowDrawList();

            static float upper = 255.0f;
            static float lower = 0.0f;
            ImGui::DragFloat("Upper", &upper, 0.2f, 0.0f, 255.0f, "%.0f");
            ImGui::DragFloat("Lower", &lower, 0.2f, 0.0f, 255.0f, "%.0f");

            const ImVec2 p = ImGui::GetCursorScreenPos();

            for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
            {
                for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                {
                    auto unscaledValue = weightMap[yIndex*xSteps + xIndex];
                    auto value = scaleColorToUCharRangeWithZoom(unscaledValue, maxValue, 0.f, static_cast<int>(upper), static_cast<int>(lower));
                    
                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
        }
        ImGui::End();
    }

    static void RenderBmuHits(const Som &som)
    {
        if (ImGui::Begin("BMU Hits"))
        {
            auto xSteps = som.getWidth();
            auto ySteps = som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto bmuHits = som.getBmuHits();
            auto maxValue = *std::max_element(bmuHits.begin(), bmuHits.end());

            ImDrawList *draw_list = ImGui::GetWindowDrawList();

            static float upper = 255.0f;
            static float lower = 0.0f;
            ImGui::DragFloat("Upper", &upper, 0.2f, 0.0f, 255.0f, "%.0f");
            ImGui::DragFloat("Lower", &lower, 0.2f, 0.0f, 255.0f, "%.0f");

            const ImVec2 p = ImGui::GetCursorScreenPos();

            for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
            {
                for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                {
                    auto unscaledValue = bmuHits[yIndex*xSteps + xIndex];
                    auto value = scaleColorToUCharRangeWithZoom(unscaledValue, maxValue, 0.f, static_cast<int>(upper), static_cast<int>(lower));
                    
                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
        }
        ImGui::End();
    }

    static void RenderMap(const Som &som, const DataSet &dataset)
    {
        if (ImGui::Begin("Map"))
        {
            auto featureNames = dataset.getNames();
            // featureNames.push_back("None");
            const auto noneIndex = featureNames.size() - 1;

            static size_t currentRedColumnId = noneIndex;
            static size_t currentGreenColumnId = noneIndex;
            static size_t currentBlueColumnId = noneIndex;
            
            RenderCombo("Red Value", featureNames, &currentRedColumnId, dataset.getName(currentRedColumnId));
            RenderCombo("Green Value", featureNames, &currentGreenColumnId, dataset.getName(currentGreenColumnId));
            RenderCombo("Blue Value", featureNames, &currentBlueColumnId, dataset.getName(currentBlueColumnId));
            
            auto xSteps = som.getWidth();
            auto ySteps = som.getHeight();
            auto xStepSize = ImGui::GetContentRegionAvail().x / xSteps;
            auto yStepSize = ImGui::GetContentRegionAvail().y / ySteps;

            auto maxRedValue = som.getMaxValueOfFeature(currentRedColumnId);
            auto minRedValue = som.getMinValueOfFeature(currentRedColumnId);
            auto maxGreenValue = som.getMaxValueOfFeature(currentGreenColumnId);
            auto minGreenValue = som.getMinValueOfFeature(currentGreenColumnId);
            auto maxBlueValue = som.getMaxValueOfFeature(currentBlueColumnId);
            auto minBlueValue = som.getMinValueOfFeature(currentBlueColumnId);

            size_t hoverNeuronX{0}, hoverNeuronY{0};

            if(ImGui::BeginChild("HoverMap"))
            {
                /* Hover neuron index */
                hoverNeuronX = static_cast<size_t>((ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX())/xStepSize);
                hoverNeuronY = static_cast<size_t>((ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY())/yStepSize);

                ImDrawList *draw_list = ImGui::GetWindowDrawList();

                const ImVec2 p = ImGui::GetCursorScreenPos();

                for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
                {
                    for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                    {
                        auto modelVector = som.getNeuron(SomIndex{xIndex, yIndex});

                        auto redValue = modelVector[currentRedColumnId];
                        auto greenValue = modelVector[currentGreenColumnId];
                        auto blueValue = modelVector[currentBlueColumnId];

                        auto constrainedRedValue = scaleColorToUCharRange(redValue, maxRedValue, minRedValue);
                        auto constrainedGreenValue = scaleColorToUCharRange(greenValue, maxGreenValue, minGreenValue);
                        auto constrainedBlueValue = scaleColorToUCharRange(blueValue, maxBlueValue, minBlueValue);

                        draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                           ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), 
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255));
                    }
                }
            }

            ImGui::EndChild();

            /* Display model vector values in tooltip */
            if (ImGui::IsItemHovered())
            {
                auto currentNeuron = som.getNeuron(som.getIndex(SomIndex{hoverNeuronX, hoverNeuronY}));

                if (!showModelVectorsAsImage)
                {
                    ImGui::BeginTooltip();
                    for (size_t i{0}; i < currentNeuron.size(); ++i)
                    {
                        auto featureName = dataset.getName(i).c_str();

                        ImGui::Text("%s:\t%.3f", featureName, currentNeuron[i]);
                    }
                    ImGui::EndTooltip();
                }
                else if (showModelVectorsAsImage)
                {
                    ImDrawList *draw_list = ImGui::GetForegroundDrawList();
                    
                    for (size_t i{0}; i < currentNeuron.size(); ++i)
                    {
                            const ImVec2 p = ImGui::GetMousePos();

                            const size_t width = modelVectorAsImageWidth, height = modelVectorAsImageWidth;
                            const size_t x_offset = 10, y_offset = 20;
                            const size_t x = i % width * 2 + x_offset;
                            const size_t y = i / height * 2 + y_offset;

                            auto currentValue = scaleColorToUCharRange(currentNeuron[i], 255.f, 0.f);

                            draw_list->AddRectFilled(ImVec2(p.x + x, p.y + y),
                                                     ImVec2(p.x + x + 2, p.y + y + 2),
                                                     IM_COL32(currentValue, currentValue, currentValue, 255));
                    }
                }
            }
        }
        ImGui::End();
    }

    static void RenderSigmaMap(const Som &som, const DataSet &dataset)
    {
        if (ImGui::Begin("Sigma Map"))
        {
            auto featureNames = dataset.getNames();
            // featureNames.push_back("None");
            const auto noneIndex = featureNames.size() - 1;

            static size_t currentRedColumnId = noneIndex;
            static size_t currentGreenColumnId = noneIndex;
            static size_t currentBlueColumnId = noneIndex;
            
            RenderCombo("Red Value", featureNames, &currentRedColumnId, dataset.getName(currentRedColumnId));
            RenderCombo("Green Value", featureNames, &currentGreenColumnId, dataset.getName(currentGreenColumnId));
            RenderCombo("Blue Value", featureNames, &currentBlueColumnId, dataset.getName(currentBlueColumnId));
            
            auto xSteps = som.getWidth();
            auto ySteps = som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto maxRedValue = som.getMaxSigmaOfFeature(currentRedColumnId);
            auto minRedValue = som.getMinSigmaOfFeature(currentRedColumnId);
            auto maxGreenValue = som.getMaxSigmaOfFeature(currentGreenColumnId);
            auto minGreenValue = som.getMinSigmaOfFeature(currentGreenColumnId);
            auto maxBlueValue = som.getMaxSigmaOfFeature(currentBlueColumnId);
            auto minBlueValue = som.getMinSigmaOfFeature(currentBlueColumnId);

            size_t hoverNeuronX{0}, hoverNeuronY{0};

            if (ImGui::BeginChild("HoverSigmaMap"))
            {
                /* Hover neuron index */
                hoverNeuronX = static_cast<size_t>((ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX())/xStepSize);
                hoverNeuronY = static_cast<size_t>((ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY())/yStepSize);

                ImDrawList *draw_list = ImGui::GetWindowDrawList();

                const ImVec2 p = ImGui::GetCursorScreenPos();

                for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
                {
                    for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                    {
                        auto modelVector = som.getSigmaNeuron(SomIndex{xIndex, yIndex});

                        auto redValue = modelVector[currentRedColumnId];
                        auto greenValue = modelVector[currentGreenColumnId];
                        auto blueValue = modelVector[currentBlueColumnId];

                        auto constrainedRedValue = scaleColorToUCharRange(redValue, maxRedValue, minRedValue);
                        auto constrainedGreenValue = scaleColorToUCharRange(greenValue, maxGreenValue, minGreenValue);
                        auto constrainedBlueValue = scaleColorToUCharRange(blueValue, maxBlueValue, minBlueValue);

                        draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                           ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize),
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255),
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255),
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255),
                                                           IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255));
                    }
                }
            }
            ImGui::EndChild();

            /* Display model vector values in tooltip */
            if (ImGui::IsItemHovered())
            {
                auto index = SomIndex{hoverNeuronX, hoverNeuronY};
                auto currentNeuron = som.getNeuron(index);
                auto currentNeuronSigma = som.getSigmaNeuron(index);

                ImGui::BeginTooltip();
                for(size_t i{0}; i<currentNeuron.size(); ++i)
                {
                    auto featureName = dataset.getName(i).c_str();

                    ImGui::Text("%s:\t%.3f +- %.3f", featureName, currentNeuron[i], currentNeuronSigma[i]);
                }

                ImGui::EndTooltip();
                
                ImGui::OpenPopup("my popup");
            }

        }
        ImGui::End();
    }

    void SomHandler(Som &som, DataSet &dataset)
    {
        static std::thread trainingThread;
        auto currentlyTraining = som.isTraining();
        if (ImGui::Begin("SOM"))
        {
            if(currentlyTraining) ImGui::BeginDisabled(true);
            {
                static int width = som.getWidth();
                static int height = som.getHeight();
                ImGui::Text("SOM");
                ImGui::InputInt("Width", &width);
                ImGui::InputInt("Height", &height);
                if (ImGui::Button("Create"))
                    som = Som(width, height, dataset.vectorLength());
                static float initSigma = 1.0f;
                ImGui::InputFloat("Init variance", &initSigma);
                if(ImGui::Button("Randomly initialize")) som.randomInitialize((unsigned)(time(NULL)+clock()), initSigma);
                
                static int numberOfEpochs = 100;
                static double eta0 = 0.9;
                static double etaDecay = 0.01;
                static double sigma0 = 10;
                static double sigmaDecay = 0.01;

                // ImGui::InputInt("Number of epochs", numberOfEpochs);
                static int elem = static_cast<std::underlying_type_t<Som::WeigthDecayFunction>>(Som::WeigthDecayFunction::Exponential);
                const char* elems_names[3] = { "Exponential", "Inverse proportional", "Batch Map" };
                const char* elem_name = (elem >= 0 && elem < 3) ? elems_names[elem] : "Unknown";
                ImGui::SliderInt("Weight decay function", &elem, 0, 2, elem_name);

                ImGui::SliderInt("Number of epochs", &numberOfEpochs, 1, 1000);
                if (elem == 0)
                {
                    ImGui::InputDouble("Eta0", &eta0);
                    ImGui::InputDouble("Eta decay", &etaDecay);
                }
                ImGui::InputDouble("Sigma0", &sigma0);
                ImGui::InputDouble("Sigma decay", &sigmaDecay);
                // #include <type_traits>
                
                if (ImGui::Button("Train"))
                {
                    trainingThread = std::thread(&Som::train, std::ref(som), std::ref(dataset), static_cast<size_t>(numberOfEpochs), eta0, etaDecay, sigma0, sigmaDecay, static_cast<Som::WeigthDecayFunction>(elem), true);
                    trainingThread.detach();
                }
                if (currentlyTraining)
                {
                    ImGui::SameLine();
                    ImGui::Text("Training SOM...");
                }
                
            }
            if(currentlyTraining) ImGui::EndDisabled();


        }
        ImGui::End();
    }

    void MetricsViewer(Som &som)
    {
        if (ImGui::Begin("Metrics"))
        {
            {
                const std::lock_guard<std::mutex> lock(som.metricsMutex);
                auto metrics = som.getMetrics();
                auto maxValue = std::max_element(metrics.MeanSquaredError.begin(), metrics.MeanSquaredError.end());
                ImGui::PlotLines("Mean Squared Training Error", metrics.MeanSquaredError.data(), metrics.MeanSquaredError.size(), 0, nullptr, 0.0f, *maxValue, ImVec2(0, 80.0f));
            }

        }
        ImGui::End();
    }

    void SettingsPane()
    {
        if(ImGui::Begin("Settings"))
        {
            ImGui::Text("Display");
            ImGui::Checkbox("Show model vectors as image", &showModelVectorsAsImage);

            if(showModelVectorsAsImage)
            {
                ImGui::InputInt("Image width", &modelVectorAsImageWidth);
                ImGui::InputInt("Image height", &modelVectorAsImageHeight);
            }
            if(modelVectorAsImageHeight < 0) modelVectorAsImageHeight = 0;
            if(modelVectorAsImageWidth < 0) modelVectorAsImageWidth = 0;
        }
        ImGui::End();
    }

    void RenderExplorer(Som &som, DataSet &dataset)
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        LoadMainMenu();

        SomHandler(som, dataset);
        SettingsPane();

        DatasetViewer(dataset);
        DatasetEditor(dataset);

        RenderUMatrix(som.getUMatrix());
        RenderWeigthMap(som);
        RenderBmuHits(som);

        MetricsViewer(som);

        RenderMap(som, dataset);
        RenderSigmaMap(som, dataset);

    }
}