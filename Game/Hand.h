#pragma once
#include <tuple> // Для использования std::tuple

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// Класс, обрабатывающий действия игрока (ввод)
class Hand
{
public:
    Hand(Board* board) : board(board) {}

    // Получение координат клетки, выбранной пользователем
    std::tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent; // Структура для хранения событий SDL
        Response resp = Response::OK; // Изначально предполагаем корректный ввод
        int x = -1, y = -1; // Координаты курсора мыши
        int xc = -1, yc = -1; // Координаты клетки на игровом поле

        // Цикл ожидания события от пользователя
        while (true)
        {
            // Получение события из очереди событий SDL
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: // Событие закрытия окна
                    resp = Response::QUIT; // Устанавливаем код ответа "выход"
                    break;
                case SDL_MOUSEBUTTONDOWN: // Событие нажатия кнопки мыши
                    // Получение координат курсора мыши
                    x = windowEvent.button.x;
                    y = windowEvent.button.y;
                    // Расчет координат клетки на игровом поле
                    xc = int(y / (board->H / 10) - 1);
                    yc = int(x / (board->W / 10) - 1);

                    // Обработка нажатия на кнопку "Назад"
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1)
                    {
                        resp = Response::BACK; // Устанавливаем код ответа "назад"
                    }
                    // Обработка нажатия на кнопку "Перезапуск"
                    else if (xc == -1 && yc == 8)
                    {
                        resp = Response::REPLAY; // Устанавливаем код ответа "перезапуск"
                    }
                    // Обработка нажатия на клетку игрового поля
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8)
                    {
                        resp = Response::CELL; // Устанавливаем код ответа "клетка"
                    }
                    else
                    {
                        xc = -1; // Сбрасываем координаты клетки, если клик был вне игрового поля
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // Событие изменения размера окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        // Обновление размера доски при изменении размера окна
                        board->reset_window_size();
                        break;
                    }
                }
                // Если получен ответ, выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }
        // Возвращаем кортеж: код ответа, координаты клетки
        return { resp, xc, yc };
    }

    // Ожидание действия игрока после окончания игры
    Response wait() const
    {
        SDL_Event windowEvent; // Структура для хранения событий SDL
        Response resp = Response::OK; // Изначально предполагаем корректный ввод

        // Цикл ожидания события от пользователя
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT: // Событие закрытия окна
                    resp = Response::QUIT; // Устанавливаем код ответа "выход"
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED: // Событие изменения размера окна
                    board->reset_window_size(); // Обновляем размер доски
                    break;
                case SDL_MOUSEBUTTONDOWN: // Событие нажатия кнопки мыши
                {
                    int x = windowEvent.button.x; // Координаты курсора мыши
                    int y = windowEvent.button.y;
                    int xc = int(y / (board->H / 10) - 1); // Координаты клетки на игровом поле
                    int yc = int(x / (board->W / 10) - 1);

                    // Обработка нажатия на кнопку "Перезапуск"
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY; // Устанавливаем код ответа "перезапуск"
                }
                break;
                }
                // Если получен ответ, выходим из цикла
                if (resp != Response::OK)
                    break;
            }
        }
        // Возвращаем код ответа
        return resp;
    }

private:
    Board* board; // Указатель на объект игрового поля
};
