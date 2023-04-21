#pragma once

#include <libsom/SOM.hpp>
#include <libsom/DataSet.hpp>
#include <libsom/SqliteDataLoader.hpp>
#include <imgui/imgui.h>
#include "ImGuiFileDialog/ImGuiFileDialog.h"

#include <optional>

namespace VSOMExplorer
{
    class Handler
    {
    private:
        std::unique_ptr<IDataLoader> m_dataLoader = std::unique_ptr<IDataLoader>();
        std::unique_ptr<DataSet> m_dataset = std::unique_ptr<DataSet>();
        Som m_som = Som(10, 10, 3);
        bool showModelVectorsAsImage = false;
        int modelVectorAsImageWidth = 28;
        int modelVectorAsImageHeight = 28;
        std::string trainingSetPath = std::string{};

        size_t m_currentRedColumnId = 0;
        size_t m_currentGreenColumnId = 0;
        size_t m_currentBlueColumnId = 0;

        static int scaleColorToUCharRange(float value, float max, float min);
        static int scaleColorToUCharRangeWithZoom(float value, float max, float min, int outMax, int outMin);
        void RenderCombo(const char *name, const char *const *labels, const size_t numberOfChoices, size_t *currentId, const char *combo_preview_value);
        void RenderCombo(const std::string &name, const std::vector<std::string> &labels, size_t *currentId, const std::string &combo_preview_value);
        void LoadMainMenu();
        void DatasetEditor();
        void DatasetViewer();
        void RenderUMatrix();
        void RenderWeigthMap();
        void RenderBmuHits();
        void RenderMap();
        void RenderSigmaMap();
        void SomHandler();
        void MetricsViewer();
        void SettingsPane();

    public:
        Handler() 
        {
            m_som.randomInitialize((unsigned)(time(NULL)+clock()), 1);
        };
        Handler(const Handler&) = delete;
        Handler& operator=(const Handler&) = delete;
        ~Handler() = default;

        void RenderExplorer();
        void SetDataset(std::unique_ptr<DataSet> dataset);
    };
}