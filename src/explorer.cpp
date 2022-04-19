#include "explorer.h"

#include <cstdio>
#include <iostream>

namespace VSOMExplorer
{
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

    static void DatasetViewer(const DataSet &dataset)
    {
        if (ImGui::Begin("Dataset"))
        {
            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
            static bool display_headers = true;
            auto numberOfColumns = dataset.vectorLength();
            auto numberOfRows = dataset.size();

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
            ImGui::End();
        }
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
                    auto staticallyScaledValue = 255.0 / maxValue * unscaledValue;
                    auto span = upper - lower;
                    auto dynamicallyScaledValue = (staticallyScaledValue - lower) * 255.0 / span;
                    auto contrainedValue = dynamicallyScaledValue < 0 ? 0 : dynamicallyScaledValue > 255.0 ? 255.0
                                                                                                           : dynamicallyScaledValue;
                    auto value = static_cast<int>(contrainedValue);
                    
                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
                }
            }
            ImGui::End();
        }
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
            auto xStepSize = ImGui::GetWindowWidth() / xSteps;
            auto yStepSize = ImGui::GetWindowHeight() / ySteps;

            auto maxRedValue = som.getMaxValueOfFeature(currentRedColumnId);
            auto minRedValue = som.getMinValueOfFeature(currentRedColumnId);
            auto maxGreenValue = som.getMaxValueOfFeature(currentGreenColumnId);
            auto minGreenValue = som.getMinValueOfFeature(currentGreenColumnId);
            auto maxBlueValue = som.getMaxValueOfFeature(currentBlueColumnId);
            auto minBlueValue = som.getMinValueOfFeature(currentBlueColumnId);

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
                    
                    auto staticallyScaledRedValue = (redValue - minRedValue)*255/(maxRedValue - minRedValue)*redValue;
                    auto staticallyScaledGreenValue = (greenValue - minGreenValue)*255/(maxGreenValue - minGreenValue)*greenValue;
                    auto staticallyScaledBlueValue = (blueValue - minBlueValue)*255/(maxBlueValue - minBlueValue)*blueValue;

                    auto constrainedRedValue = static_cast<int>(staticallyScaledRedValue > 255.0 ? 255.0 :
                        staticallyScaledRedValue < 0.0 ? 0.0 : 
                        staticallyScaledRedValue);
                    auto constrainedGreenValue = static_cast<int>(staticallyScaledGreenValue > 255.0 ? 255.0 :
                        staticallyScaledGreenValue < 0.0 ? 0.0 : 
                        staticallyScaledGreenValue);
                    auto constrainedBlueValue = static_cast<int>(staticallyScaledBlueValue > 255.0 ? 255.0 :
                        staticallyScaledBlueValue < 0.0 ? 0.0 : 
                        staticallyScaledBlueValue);

                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex * xStepSize, p.y + yIndex * yStepSize),
                                                       ImVec2(p.x + (xIndex + 1) * xStepSize, p.y + (yIndex + 1) * yStepSize), 
                                                       IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                       IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                       IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255), 
                                                       IM_COL32(constrainedRedValue, constrainedGreenValue, constrainedBlueValue, 255));
                }
            }

            ImGui::End();
        }
    }

    void RenderExplorer(const Som &som, const DataSet &dataset)
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        LoadMainMenu();

        DatasetViewer(dataset);

        RenderUMatrix(som.getUMatrix());

        RenderMap(som, dataset);
    }
}