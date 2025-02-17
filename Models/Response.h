#pragma once

// Перечисление возможных ответов от пользователя
enum class Response {
    OK,      // Ход успешно выполнен
    BACK,    // Возврат к предыдущему состоянию
    REPLAY,  // Начать игру заново
    QUIT,    // Выход из игры
    CELL     // Выбрана клетка на игровом поле
};
