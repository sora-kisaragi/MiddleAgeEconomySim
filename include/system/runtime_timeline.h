#pragma once
#include "core/economy_types.h"
#include <locale>
#include <ostream>

// Diagnostic identity/time trace, not a settlement journal. Stop the run after
// any write failure; partial rows are not recoverable committed records.
class RuntimeTimeline {
public:
    RuntimeTimeline(std::ostream& out, std::vector<economy::ActorId> actors) : out_(out) {
        std::sort(actors.begin(), actors.end());
        if (std::adjacent_find(actors.begin(), actors.end()) != actors.end()) {
            throw std::invalid_argument("duplicate timeline actor");
        }
        out_.imbue(std::locale::classic());
        out_ << "event_id,event_type,actor_id,day,phase,sequence_in_day\n";
        check();
        std::uint64_t sequence = 0;
        for (auto actor : actors) {
            write("ACTOR_REGISTERED", actor, economy::GameTime(0, 0, ++sequence));
        }
    }

    economy::GameTime beginDay(std::uint64_t day) {
        if (open_day_ || day != completed_day_ + 1) {
            throw std::invalid_argument("timeline days must be consecutive and nonoverlapping");
        }
        const auto time = clock_.next(day, 1);
        write("DAY_STARTED", std::nullopt, time);
        open_day_ = day;
        return time;
    }

    void completeDay(std::uint64_t day) {
        if (!open_day_ || *open_day_ != day) {
            throw std::invalid_argument("timeline completion does not match open day");
        }
        write("DAY_COMPLETED", std::nullopt, clock_.next(day, 12));
        completed_day_ = day;
        open_day_.reset();
    }

private:
    std::ostream& out_;
    economy::GameTimeSequencer clock_;
    std::uint64_t next_event_ = 1;
    std::uint64_t completed_day_ = 0;
    std::optional<std::uint64_t> open_day_;

    void check() {
        out_.flush();
        if (!out_) throw std::runtime_error("Cannot write runtime timeline CSV");
    }

    void write(const char* type, std::optional<economy::ActorId> actor,
               const economy::GameTime& time) {
        const economy::EventId event(next_event_);
        out_ << std::dec << event.value() << ',' << type << ',';
        if (actor) out_ << actor->value();
        out_ << ',' << time.day << ',' << static_cast<unsigned int>(time.phase)
             << ',' << time.sequence_in_day << '\n';
        check();
        ++next_event_;
    }
};
