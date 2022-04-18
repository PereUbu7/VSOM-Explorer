#include "explorer.h"

#include <cstdio>
#include <iostream>

namespace VSOMExplorer
{
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
            // Expose a few Borders related flags interactively
            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
            static bool display_headers = true;
            auto numberOfColumns = dataset.vectorLength();
            auto numberOfRows = dataset.size();

            if (ImGui::BeginTable("table1", numberOfColumns, flags))
            {
                // Display headers so we can inspect their interaction with borders.
                // (Headers are not the main purpose of this section of the demo, so we are not elaborating on them too much. See other sections for details)
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
            auto minValue = uMatrix.getMinValue();

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
                    auto staticallyScaledValue = 255.0/maxValue*unscaledValue;
                    auto span = upper - lower;
                    auto dynamicallyScaledValue = (staticallyScaledValue - lower)*255.0/span;
                    auto contrainedValue = dynamicallyScaledValue < 0 ? 0 :
                        dynamicallyScaledValue > 255.0 ? 255.0 : dynamicallyScaledValue;
                    auto value = static_cast<int>(contrainedValue);
                    // std::cout << "Value is:" << value << '\n';
                    draw_list->AddRectFilledMultiColor(ImVec2(p.x + xIndex*xStepSize, p.y + yIndex*yStepSize), 
                        ImVec2(p.x + (xIndex+1)*xStepSize, p.y + (yIndex+1)*yStepSize), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255), IM_COL32(value, value, value, 255));
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
    }
}