/*
 * Created by lebakassemmerl 2023
 * E-Mail: hotschi@gmx.at
 * 
 * This EventTimer configures a hardware-timer to fire an interrupt every 1ms. The API is designed
 * to register multiple Events which will fire after a configured amount of milliseconds (interval).
 * Be aware, that the callbacks of this EventTimer are running in interrupt-context!
 * 
 * The API is only safe to use if the events are registered outside of an interrupt-context,
 * otherwise this could end up in undefined behavior.
 */

#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <expected>
#include <utility>

#include "cs.h"
#include "timer32.h"

class EventTimer {
public:
    class Event {
    public:
        // we don't want an event to be copied
        constexpr explicit Event(uint8_t idx) noexcept : ev(idx) {}

        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;
        constexpr Event(const Event&& other) noexcept : ev(std::move(other.ev)) {}
        constexpr Event& operator=(const Event&& other) = delete;
        friend class EventTimer;
    private:
        uint8_t ev;
    };

    constexpr explicit EventTimer(Timer32& t32) noexcept
        : initialized(false), ev_list(), t32(t32) {}

    void init(const Cs& cs) noexcept;
    Err start_event(const Event& ev) noexcept;
    Err stop_event(const Event& ev) noexcept;
    std::expected<EventTimer::Event, Err> register_event(
        uint16_t interval_ms, void (*elapsed_cb)(void* cookie) noexcept, void* cookie) noexcept;
private:
    struct EventEntry {
        constexpr explicit EventEntry() noexcept
            : interval(0), cnt(0), cookie(nullptr), elapsed(nullptr) {}

        uint16_t interval;
        uint16_t cnt;
        void* cookie;
        void (*elapsed)(void* cookie) noexcept;
    };

    template<typename T> requires std::unsigned_integral<T>
    struct EventList {
        constexpr explicit EventList() noexcept : enabled(0), used(0), events() {}

        std::atomic<T> enabled;
        std::atomic<uint8_t> used;
        std::array<EventEntry, sizeof(T) * 8> events;
    };

    static void timer_cb(void* cookie) noexcept;

    bool initialized;
    EventList<uint32_t> ev_list;
    Timer32& t32;
};
