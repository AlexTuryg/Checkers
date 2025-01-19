#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    Config()
    {
        // Загружаем конфигурацию при создании объекта.
        reload();
    }

    // Перечитывает файл конфигурации settings.json.
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        // Проверяем, успешно ли открылся файл.
        if (!fin.is_open()) {
            // Обработка ошибки:  Можно вывести сообщение об ошибке или использовать значения по умолчанию.
            std::cerr << "Ошибка открытия файла конфигурации settings.json" << std::endl;
            // В качестве примера, используем пустой JSON, если файл не найден.
            config = json::object();
            return;
        }
        // Парсим JSON из файла в объект config.
        fin >> config;
        fin.close();
    }

    // Возвращает значение настройки по указанному пути.  Используется перегрузка оператора ().
    auto operator()(const string& setting_dir, const string& setting_name) const
    {
        //  Обработка возможных ошибок: проверка существования ключей в JSON.
        if (!config.contains(setting_dir) || !config[setting_dir].contains(setting_name)) {
            // Возвращаем значение по умолчанию или выбрасываем исключение, в зависимости от требований.
            // Здесь возвращаем пустую строку в качестве значения по умолчанию.
            return std::string("");
        }
        return config[setting_dir][setting_name];
    }

private:
    // Объект JSON для хранения конфигурации.
    json config;
};
