#pragma once
#include <chrono> // Библиотека для работы со временем
#include <thread> // Библиотека для работы с потоками

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        // Создаем и очищаем лог-файл
        std::ofstream fout(project_path + "log.txt", std::ios_base::trunc);
        fout.close();
    }

    // Запуск игры. Возвращает код результата игры
    int play()
    
        // Запоминаем время начала игры
        auto start = std::chrono::steady_clock::now();
		
        // Обработка перезапуска игры
        if (is_replay)
        {
            // Обновляем логику, конфигурацию и перерисовываем доску
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            // Начальная отрисовка доски
            board.start_draw();
        }
        is_replay = false; // Сбрасываем флаг перезапуска

        int turn_num = -1; // Счетчик номера хода
        bool is_quit = false; // Флаг выхода из игры
        const int Max_turns = config("Game", "MaxNumTurns"); // Максимальное количество ходов

        // Игровой цикл, ограниченный максимальным количеством ходов
        while (++turn_num < Max_turns)
        {
            beat_series = 0; // Сбрасываем счетчик серии взятий

            // Поиск возможных ходов для текущего игрока
            logic.find_turns(turn_num % 2);
            // Если ходов нет, игра заканчивается
            if (logic.turns.empty())
                break;

            // Устанавливаем глубину поиска для бота в зависимости от цвета
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));

            // Ход игрока или бота
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                // Ход игрока
                auto resp = player_turn(turn_num % 2);
                if (resp == Response::QUIT) // Выход из игры
                {
                    is_quit = true;
                    break;
                }
                else if (resp == Response::REPLAY) // Перезапуск игры
                {
                    is_replay = true;
                    break;
                }
                else if (resp == Response::BACK) // Отмена хода
                {
                    // Проверка возможности отмены хода (бот, серия взятий, количество ходов)
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        // Откат доски на один ход
                        board.rollback();
                        --turn_num;
                    }
                    // Если серии взятий не было, отменяем ход
                    if (!beat_series)
                        --turn_num;
                    // Откат доски
                    board.rollback();
                    --turn_num;
                    // Сбрасываем счетчик серии взятий
                    beat_series = 0;
                }
            }
            else
            {
                // Ход бота
                bot_turn(turn_num % 2);
            }
        }
        // Запоминаем время окончания игры
        auto end = std::chrono::steady_clock::now();
        std::ofstream fout(project_path + "log.txt", std::ios_base::app);
        // Записываем время игры в лог-файл
        fout << "Game time: " << (int)std::chrono::duration<double, std::milli>(end - start).count() << " millisec\n";
        fout.close();

        // Обработка перезапуска или выхода из игры.
        if (is_replay)
            return play();
        if (is_quit)
            return 0;

        // Определение результата игры.
        int res = 2; // Предполагается победа белых.
        if (turn_num == Max_turns)
        {
            res = 0; // Ничья.
        }
        else if (turn_num % 2)
        {
            res = 1; // Победа черных.
        }
        // Отображение финальной картинки.
        board.show_final(res);
        // Ожидание действия пользователя.
        auto resp = hand.wait();
        // Обработка перезапуска игры пользователем.
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play(); // Перезапускаем игру.
        }
        return res; // Возвращаем результат игры.
    }

private:
    // Ход бота.
    void bot_turn(const bool color)
    {
        // Запоминаем время начала хода.
        auto start = std::chrono::steady_clock::now();

        auto delay_ms = config("Bot", "BotDelayMS");
        // Создаем новый поток для задержки хода.
        std::thread th(SDL_Delay, delay_ms);
        // Находим лучшие ходы для бота.
        auto turns = logic.find_best_turns(color);
        // Ожидаем завершения потока задержки.
        th.join();
        bool is_first = true;
        // Выполнение ходов бота.
        for (auto turn : turns)
        {
            // Задержка между ходами (кроме первого).
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            // Увеличиваем счетчик серии взятий, если была съедена фигура.
            beat_series += (turn.xb != -1);
            // Выполняем ход на доске.
            board.move_piece(turn, beat_series);
        }
        // Запоминаем время окончания хода.
        auto end = std::chrono::steady_clock::now();
        // Записываем время хода в лог-файл.
        std::ofstream fout(project_path + "log.txt", std::ios_base::app);
        fout << "Bot turn time: " << (int)std::chrono::duration<double, std::milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    // Ход игрока.
    Response player_turn(const bool color)
    {
        // Возвращает 1, если игрок вышел из игры.
        std::vector<std::pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        // Подсветка доступных клеток для хода.
        board.highlight_cells(cells);
        // Инициализация текущих координат.
        move_pos pos = { -1, -1, -1, -1 };
        POS_T x = -1, y = -1;
        // Ожидание первого хода игрока.
        while (true)
        {
            // Получение координат выбранной клетки от игрока.
            auto resp = hand.get_cell();
            // Возвращаем ответ, если это не клетка.
            if (std::get<0>(resp) != Response::CELL)
                return std::get<0>(resp);
            // Координаты выбранной клетки.
            std::pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };

            bool is_correct = false;
            // Проверка, является ли клетка доступной для хода.
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                // Если выбрана фигура и конечная клетка хода.
                if (turn == move_pos{ x, y, cell.first, cell.second })
                {
                    pos = turn;
                    break;
                }
            }
            // Если найден правильный ход, выходим из цикла.
            if (pos.x != -1)
                break;
            // Если ход некорректный, сбрасываем координаты.
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            // Очищаем выделения.
            board.clear_highlight();
            // Выделяем текущую клетку как активную.
            board.set_active(x, y);
            // Собираем клетки для хода.
            std::vector<std::pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            // Подсвечиваем клетки для хода.
            board.highlight_cells(cells2);
        }

        // Очищаем подсветку.
        board.clear_highlight();
        board.clear_active();
        // Перемещаем шашку.
        board.move_piece(pos, pos.xb != -1);
        // Если нет съедаемых фигур, возвращаем ОК.
        if (pos.xb == -1)
            return Response::OK;
        // Продолжаем бить, пока есть возможность.
        beat_series = 1;
        while (true)
        {
            // Находим съедаемые фигуры.
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            // Собираем список клеток для дальнейшего хода.
            std::vector<std::pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            // Подсвечиваем клетки.
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // Ожидание хода игрока.
            while (true)
            {
                // Действие пользователя.
                auto resp = hand.get_cell();
                // Выход из игры.
                if (std::get<0>(resp) != Response::CELL)
                    return std::get<0>(resp);
                // Получаем координаты выбранной клетки.
                std::pair<POS_T, POS_T> cell{ std::get<1>(resp), std::get<2>(resp) };

                bool is_correct = false;
                // Проверяем выбранную клетку на корректность.
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                // Если клетка некорректна, повторяем процесс.
                if (!is_correct)
                    continue;
                // Очищаем подсветку.
                board.clear_highlight();
                board.clear_active();
                // Увеличиваем счетчик серии взятий.
                beat_series += 1;
                // Перемещаем фигуру.
                board.move_piece(pos, beat_series);
                break;
            }
        }
        // Возвращаем ОК.
        return Response::OK;
    }

private:
    Config config; // Объект конфигурации.
    Board board; // Объект игрового поля.
    Hand hand; // Объект управления вводом.
    Logic logic; // Объект игровой логики.
    int beat_series; // Счетчик серии взятий.
    bool is_replay = false; // Флаг перезапуска игры.
};
