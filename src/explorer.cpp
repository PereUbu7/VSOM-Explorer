#include "explorer.h"

#include <cstdio>
#include <iostream>
#include <thread>
#include <functional>
#include <algorithm>

namespace VSOMExplorer
{
    int Handler::scaleColorToUCharRange(float value, float max, float min)
    {
        auto staticallyScaledValue = (value - min) * 255 / (max - min);

        return static_cast<int>(staticallyScaledValue > 255.f ? 255.f : staticallyScaledValue < 0.f ? 0.f
                                                                                                    : staticallyScaledValue);
    };

    int Handler::scaleColorToUCharRangeWithZoom(float value, float max, float min, int outMax, int outMin)
    {
        auto scaledValue = scaleColorToUCharRange(value, max, min);

        auto span = outMax - outMin;
        auto dynamicallyScaledValue = (scaledValue - outMin) * 255.0 / span;
        auto contrainedValue = dynamicallyScaledValue < 0. ? 0. : dynamicallyScaledValue > 255.0 ? 255.0
                                                                                                 : dynamicallyScaledValue;
        return static_cast<int>(contrainedValue);
    }

    void Handler::RenderCombo(const char *name, const char *const *labels, const size_t numberOfChoices, size_t *currentId, const char *combo_preview_value)
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

    void Handler::RenderCombo(const std::string &name, const std::vector<std::string> &labels, size_t *currentId, const std::string &combo_preview_value)
    {
        // Convert vector<string> to const char* const*
        const std::vector<const char *> c_labels = [&labels]()
        {
            auto c_l = std::vector<const char *>{};
            c_l.reserve(labels.size());

            for (auto &s : labels)
                c_l.push_back(&s[0]);

            return c_l;
        }();

        RenderCombo(name.c_str(), c_labels.data(), labels.size(), currentId, combo_preview_value.c_str());
    }

    void Handler::LoadMainMenu()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "CTRL+N"))
                {
                }
                if (ImGui::MenuItem("Open data", "CTRL+O"))
                {
                    // open Dialog Simple
                    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", "*,*.*", ".", 1, nullptr, ImGuiFileDialogFlags_Modal);
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

        // display
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
        {
            // action if OK
            if (ImGuiFileDialog::Instance()->IsOk())
            {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();

                std::optional<size_t> lastDatasetDepth = m_dataLoader ? m_dataLoader->getDepth() : std::optional<size_t>{};

                m_dataLoader = std::make_unique<SqliteDataLoader>("../data/columnSpec.txt");

                /* Open new dataset */
                if (m_dataLoader->open(filePathName.c_str()) != 0) 
                {
                    m_dataset = std::unique_ptr<DataSet>(new DataSet(*m_dataLoader));
                    m_som = Som(10, 10, m_dataset->vectorLength());
                }
            }

            // close
            ImGuiFileDialog::Instance()->Close();
        }
    }

    void Handler::DatasetEditor()
    {
        if (ImGui::Begin("Dataset Editor") && m_dataset != nullptr)
        {
            auto numberOfColumns = m_dataset->vectorLength();
            auto columnNames = m_dataset->getNames();

            static float setAllValue{0};
            ImGui::InputFloat("Set all", &setAllValue);
            if (ImGui::Button("Apply"))
            {
                for (size_t currentColumn{0}; currentColumn < numberOfColumns; ++currentColumn)
                {
                    m_dataset->getWeight(currentColumn) = setAllValue;
                }
            }

            ImGui::Text("Columns:");

            for (size_t currentColumn{0}; currentColumn < numberOfColumns; ++currentColumn)
            {
                ImGui::InputFloat((columnNames[currentColumn] + " Weight").c_str(), &m_dataset->getWeight(currentColumn));
            }
        }
        ImGui::End();
    }

    void Handler::DatasetViewer()
    {
        if (ImGui::Begin("Dataset") && m_dataset != nullptr)
        { // TODO: Se till så att preview data hämtas när datasetet laddas
            const static auto previewData = m_dataset->getPreviewData(100);
            auto numberOfRows = m_dataset->size();
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
                    auto currentNeuron = previewData[n];

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
                    float last_image_x2 = p.x + (modelVectorAsImageWidth * step_size + x_offset);
                    float next_image_x2 = last_image_x2 + step_size * modelVectorAsImageWidth; // Expected position if next image was on same line
                    if (next_image_x2 > window_visible_x2)
                    {
                        p.y += modelVectorAsImageHeight * step_size + y_offset;
                        p.x = original_p_x;
                    }
                }
            }
            else
            {
                static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
                static bool display_headers = true;
                auto numberOfColumns = m_dataset->vectorLength();
                numberOfColumns = numberOfColumns >= 64 ? 64 : numberOfColumns;

                if (ImGui::BeginTable("table1", numberOfColumns, flags))
                {
                    if (display_headers)
                    {
                        for (size_t columnIndex{0}; columnIndex < numberOfColumns; ++columnIndex)
                            ImGui::TableSetupColumn(m_dataset->getName(columnIndex).c_str());
                        ImGui::TableHeadersRow();
                    }

                    for (size_t row{0}; row < numberOfRows; ++row)
                    {
                        auto rowValues = previewData[row];
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

    void Handler::RenderUMatrix()
    {
        if (ImGui::Begin("U-matrix"))
        {
            auto uMatrix = m_som.getUMatrix();

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

    void Handler::RenderWeigthMap()
    {
        if (ImGui::Begin("Weight Map"))
        {
            auto xSteps = m_som.getWidth();
            auto ySteps = m_som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto weightMap = m_som.getWeigthMap();
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
                    auto unscaledValue = weightMap[yIndex * xSteps + xIndex];
                    auto value = scaleColorToUCharRangeWithZoom(unscaledValue, maxValue, 0.f, static_cast<int>(upper), static_cast<int>(lower));

                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
        }
        ImGui::End();
    }

    void Handler::RenderBmuHits()
    {
        if (ImGui::Begin("BMU Hits"))
        {
            auto xSteps = m_som.getWidth();
            auto ySteps = m_som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto bmuHits = m_som.getBmuHits();
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
                    auto unscaledValue = bmuHits[yIndex * xSteps + xIndex];
                    auto value = scaleColorToUCharRangeWithZoom(unscaledValue, maxValue, 0.f, static_cast<int>(upper), static_cast<int>(lower));

                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
        }
        ImGui::End();
    }

    void Handler::RenderMap()
    {
        if (ImGui::Begin("Map") && m_dataset != nullptr)
        {
            const auto featureNames = m_dataset->getNames();
            // featureNames.push_back("None");
            // const auto noneIndex = featureNames.size() - 1;

            // m_currentRedColumnId = 0;
            // m_currentGreenColumnId = 0;
            // m_currentGreenColumnId = 0;

            RenderCombo("Red Value", featureNames, &m_currentRedColumnId, m_dataset->getName(m_currentRedColumnId));
            RenderCombo("Green Value", featureNames, &m_currentGreenColumnId, m_dataset->getName(m_currentGreenColumnId));
            RenderCombo("Blue Value", featureNames, &m_currentBlueColumnId, m_dataset->getName(m_currentBlueColumnId));

            auto xSteps = m_som.getWidth();
            auto ySteps = m_som.getHeight();
            auto xStepSize = ImGui::GetContentRegionAvail().x / xSteps;
            auto yStepSize = ImGui::GetContentRegionAvail().y / ySteps;

            auto maxRedValue = m_som.getMaxValueOfFeature(m_currentRedColumnId);
            auto minRedValue = m_som.getMinValueOfFeature(m_currentRedColumnId);
            auto maxGreenValue = m_som.getMaxValueOfFeature(m_currentGreenColumnId);
            auto minGreenValue = m_som.getMinValueOfFeature(m_currentGreenColumnId);
            auto maxBlueValue = m_som.getMaxValueOfFeature(m_currentBlueColumnId);
            auto minBlueValue = m_som.getMinValueOfFeature(m_currentBlueColumnId);

            size_t hoverNeuronX{0}, hoverNeuronY{0};

            if (ImGui::BeginChild("HoverMap"))
            {
                /* Hover neuron index */
                hoverNeuronX = static_cast<size_t>((ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX()) / xStepSize);
                hoverNeuronY = static_cast<size_t>((ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY()) / yStepSize);

                ImDrawList *draw_list = ImGui::GetWindowDrawList();

                const ImVec2 p = ImGui::GetCursorScreenPos();

                for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
                {
                    for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                    {
                        auto modelVector = m_som.getNeuron(SomIndex{xIndex, yIndex});

                        auto redValue = modelVector[m_currentRedColumnId];
                        auto greenValue = modelVector[m_currentGreenColumnId];
                        auto blueValue = modelVector[m_currentBlueColumnId];

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
                auto currentNeuron = m_som.getNeuron(m_som.getIndex(SomIndex{hoverNeuronX, hoverNeuronY}));

                if (!showModelVectorsAsImage)
                {
                    ImGui::BeginTooltip();
                    for (size_t i{0}; i < currentNeuron.size(); ++i)
                    {
                        auto featureName = m_dataset->getName(i).c_str();

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

    void Handler::RenderSigmaMap()
    {
        if (ImGui::Begin("Sigma Map") && m_dataset != nullptr)
        {
            auto featureNames = m_dataset->getNames();
            // featureNames.push_back("None");
            // const auto noneIndex = featureNames.size() - 1;

            // m_currentRedColumnId = 0;
            // m_currentGreenColumnId = 0;
            // m_currentGreenColumnId = 0;

            RenderCombo("Red Value", featureNames, &m_currentRedColumnId, m_dataset->getName(m_currentRedColumnId));
            RenderCombo("Green Value", featureNames, &m_currentGreenColumnId, m_dataset->getName(m_currentGreenColumnId));
            RenderCombo("Blue Value", featureNames, &m_currentBlueColumnId, m_dataset->getName(m_currentBlueColumnId));

            auto xSteps = m_som.getWidth();
            auto ySteps = m_som.getHeight();
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto maxRedValue = m_som.getMaxSigmaOfFeature(m_currentRedColumnId);
            auto minRedValue = m_som.getMinSigmaOfFeature(m_currentRedColumnId);
            auto maxGreenValue = m_som.getMaxSigmaOfFeature(m_currentGreenColumnId);
            auto minGreenValue = m_som.getMinSigmaOfFeature(m_currentGreenColumnId);
            auto maxBlueValue = m_som.getMaxSigmaOfFeature(m_currentBlueColumnId);
            auto minBlueValue = m_som.getMinSigmaOfFeature(m_currentBlueColumnId);

            size_t hoverNeuronX{0}, hoverNeuronY{0};

            if (ImGui::BeginChild("HoverSigmaMap"))
            {
                /* Hover neuron index */
                hoverNeuronX = static_cast<size_t>((ImGui::GetMousePos().x - ImGui::GetCursorScreenPos().x - ImGui::GetScrollX()) / xStepSize);
                hoverNeuronY = static_cast<size_t>((ImGui::GetMousePos().y - ImGui::GetCursorScreenPos().y - ImGui::GetScrollY()) / yStepSize);

                ImDrawList *draw_list = ImGui::GetWindowDrawList();

                const ImVec2 p = ImGui::GetCursorScreenPos();

                for (size_t yIndex{0}; yIndex < ySteps; ++yIndex)
                {
                    for (size_t xIndex{0}; xIndex < xSteps; ++xIndex)
                    {
                        auto modelVector = m_som.getSigmaNeuron(SomIndex{xIndex, yIndex});

                        auto redValue = modelVector[m_currentRedColumnId];
                        auto greenValue = modelVector[m_currentGreenColumnId];
                        auto blueValue = modelVector[m_currentBlueColumnId];

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
                auto currentNeuron = m_som.getNeuron(index);
                auto currentNeuronSigma = m_som.getSigmaNeuron(index);

                if (!showModelVectorsAsImage)
                {
                    ImGui::BeginTooltip();
                    for (size_t i{0}; i < currentNeuron.size(); ++i)
                    {
                        auto featureName = m_dataset->getName(i).c_str();

                        ImGui::Text("%s:\t%.3f +- %.3f", featureName, currentNeuron[i], currentNeuronSigma[i]);
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

                        auto currentValue = scaleColorToUCharRange(currentNeuronSigma[i], 255.f, 0.f);

                        draw_list->AddRectFilled(ImVec2(p.x + x, p.y + y),
                                                 ImVec2(p.x + x + 2, p.y + y + 2),
                                                 IM_COL32(currentValue, currentValue, currentValue, 255));
                    }
                }
            }
        }
        ImGui::End();
    }

    void Handler::SomHandler()
    {
        static std::thread trainingThread;
        auto currentlyTraining = m_som.isTraining();
        if (ImGui::Begin("SOM"))
        {
            if (currentlyTraining)
                ImGui::BeginDisabled(true);
            {
                static int width = m_som.getWidth();
                static int height = m_som.getHeight();
                ImGui::Text("SOM");
                ImGui::InputInt("Width", &width);
                ImGui::InputInt("Height", &height);
                if (ImGui::Button("Create") && m_dataset != nullptr)
                    m_som = Som(width, height, m_dataset->vectorLength());
                static float initSigma = 1.0f;
                ImGui::InputFloat("Init variance", &initSigma);
                if (ImGui::Button("Randomly initialize"))
                    m_som.randomInitialize((unsigned)(time(NULL) + clock()), initSigma);

                static int numberOfEpochs = 100;
                static double eta0 = 0.9;
                static double etaDecay = 0.01;
                static double sigma0 = 10;
                static double sigmaDecay = 0.01;

                // ImGui::InputInt("Number of epochs", numberOfEpochs);
                static int elem = static_cast<std::underlying_type_t<Som::WeigthDecayFunction>>(Som::WeigthDecayFunction::Exponential);
                const char *elems_names[3] = {"Exponential", "Inverse proportional", "Batch Map"};
                const char *elem_name = (elem >= 0 && elem < 3) ? elems_names[elem] : "Unknown";
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

                if (ImGui::Button("Train") && m_dataset != nullptr)
                {
                    trainingThread = std::thread(&Som::train, std::ref(m_som), std::ref(*m_dataset), static_cast<size_t>(numberOfEpochs), eta0, etaDecay, sigma0, sigmaDecay, static_cast<Som::WeigthDecayFunction>(elem), true);
                    trainingThread.detach();
                }
                if (currentlyTraining)
                {
                    ImGui::SameLine();
                    ImGui::Text("Training SOM...");
                }
            }
            if (currentlyTraining)
                ImGui::EndDisabled();
        }
        ImGui::End();
    }

    void Handler::MetricsViewer()
    {
        if (ImGui::Begin("Metrics"))
        {
            {
                const std::lock_guard<std::mutex> lock(m_som.metricsMutex);
                auto metrics = m_som.getMetrics();
                auto maxValue = std::max_element(metrics.MeanSquaredError.begin(), metrics.MeanSquaredError.end());
                ImGui::PlotLines("Mean Squared Training Error", metrics.MeanSquaredError.data(), metrics.MeanSquaredError.size(), 0, nullptr, 0.0f, *maxValue, ImVec2(0, 80.0f));
            }
        }
        ImGui::End();
    }

    void Handler::SettingsPane()
    {
        if (ImGui::Begin("Settings"))
        {
            ImGui::Text("Display");
            ImGui::Checkbox("Show model vectors as image", &showModelVectorsAsImage);

            if (showModelVectorsAsImage)
            {
                ImGui::InputInt("Image width", &modelVectorAsImageWidth);
                ImGui::InputInt("Image height", &modelVectorAsImageHeight);
            }
            if (modelVectorAsImageHeight < 0)
                modelVectorAsImageHeight = 0;
            if (modelVectorAsImageWidth < 0)
                modelVectorAsImageWidth = 0;
        }
        ImGui::End();
    }

    void Handler::SetDataset(std::unique_ptr<DataSet> dataset)
    {
        m_dataset = std::unique_ptr<DataSet>{std::move(dataset)};
        m_som = Som(10, 10, m_dataset->vectorLength());
    }

    void Handler::RenderExplorer()
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        try
        {
            LoadMainMenu();

            SomHandler();
            SettingsPane();

            DatasetViewer();
            DatasetEditor();

            RenderUMatrix();
            RenderWeigthMap();
            RenderBmuHits();

            MetricsViewer();

            RenderMap();
            RenderSigmaMap();
        }
        catch (const std::exception &e)
        {
            std::cerr << e.what() << '\n';
        }
        catch(...)
        {
            ;
        }
    }
}