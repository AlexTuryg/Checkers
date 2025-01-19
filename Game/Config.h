#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        // ��������� ������������ ��� �������� �������.
        reload();
    }

    // ������������ ���� ������������ settings.json.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        // ���������, ������� �� �������� ����.
        if (!fin.is_open()) {
            // ��������� ������:  ����� ������� ��������� �� ������ ��� ������������ �������� �� ���������.
            std::cerr << "������ �������� ����� ������������ settings.json" << std::endl;
            // � �������� �������, ���������� ������ JSON, ���� ���� �� ������.
            config = json::object();
            return;
        }
        // ������ JSON �� ����� � ������ config.
        fin >> config;
        fin.close();
    }

    // ���������� �������� ��������� �� ���������� ����.  ������������ ���������� ��������� ().
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        //  ��������� ��������� ������: �������� ������������� ������ � JSON.
        if (!config.contains(setting_dir) || !config[setting_dir].contains(setting_name)) {
            // ���������� �������� �� ��������� ��� ����������� ����������, � ����������� �� ����������.
            // ����� ���������� ������ ������ � �������� �������� �� ���������.
            return std::string("");
        }
        return config[setting_dir][setting_name];
    }

private:
    // ������ JSON ��� �������� ������������.
    json config;
};
