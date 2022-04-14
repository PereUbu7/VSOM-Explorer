#include "explorer.h"

#include <cstdio>

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

    static void DatasetViewer(const DataSet& dataset)
    {
        if(ImGui::Begin("Dataset"))
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
                    for(size_t columnIndex{0}; columnIndex<numberOfColumns;++columnIndex)
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

    static void RenderUMatrix(const Som& som)
    {
        if(ImGui::Begin("U-matrix"))
        {
            
            ImGui::End();
        }
    }

    void RenderExplorer(const Som& som, const DataSet& dataset)
    {
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

        LoadMainMenu();

        DatasetViewer(dataset);

        RenderUMatrix(som);
    }
}