#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct Target {
  std::string name;
  std::string domain;
  double difficulty;
  double impact;
  int attempts = 0;
  bool completed = false;
};

class Agent {
 public:
  Agent()
      : skills_({{"automation", 0.45},
                 {"debugging", 0.55},
                 {"design", 0.35},
                 {"docs", 0.40}}) {}

  Target* ChooseTarget(std::vector<Target>& targets) {
    Target* best = nullptr;
    double best_score = -1.0;

    for (auto& target : targets) {
      if (target.completed) {
        continue;
      }

      const double skill = skills_[target.domain];
      const double curiosity_bonus = target.attempts == 0 ? 0.15 : 0.0;
      const double persistence_bonus = target.attempts * 0.08;
      const double score = (target.impact * 1.2) + skill + curiosity_bonus +
                           persistence_bonus - target.difficulty;

      if (score > best_score) {
        best_score = score;
        best = &target;
      }
    }

    return best;
  }

  bool Work(Target& target) {
    ++target.attempts;
    const double readiness = skills_[target.domain] + (target.attempts - 1) * 0.18;
    const bool success = readiness >= target.difficulty;

    if (success) {
      target.completed = true;
      skills_[target.domain] = std::min(1.0, skills_[target.domain] + 0.12);
      footprints_.push_back("👣 " + target.name);
      Trim(footprints_);
    } else {
      skills_[target.domain] = std::min(1.0, skills_[target.domain] + 0.22);
      std::ostringstream lesson;
      lesson << "missed " << target.name << ", raised " << target.domain
             << " skill to " << Format(skills_[target.domain]);
      lessons_.push_back(lesson.str());
      Trim(lessons_);
    }

    return success;
  }

  void PrintStep(const Target& target, bool success) const {
    std::cout << "- chose: " << target.name << " [" << target.domain << "] -> "
              << (success ? "progress made" : "stumbled, learned, retry later")
              << '\n';
  }

  void PrintSummary(const std::vector<Target>& targets) const {
    const auto completed = std::count_if(targets.begin(), targets.end(),
                                         [](const Target& target) {
                                           return target.completed;
                                         });

    std::cout << "\nsummary\n";
    std::cout << "completed " << completed << " of " << targets.size()
              << " targets\n";

    if (!lessons_.empty()) {
      std::cout << "recent lessons\n";
      for (const auto& lesson : lessons_) {
        std::cout << "  - " << lesson << '\n';
      }
    }

    if (!footprints_.empty()) {
      std::cout << "light footprints\n";
      for (const auto& footprint : footprints_) {
        std::cout << "  " << footprint << '\n';
      }
    }
  }

 private:
  template <typename T>
  void Trim(std::vector<T>& entries) const {
    constexpr std::size_t kMaxEntries = 4;
    if (entries.size() > kMaxEntries) {
      entries.erase(entries.begin(), entries.begin() + (entries.size() - kMaxEntries));
    }
  }

  std::string Format(double value) const {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
  }

  std::unordered_map<std::string, double> skills_;
  std::vector<std::string> lessons_;
  std::vector<std::string> footprints_;
};

int main() {
  std::vector<Target> targets = {
      {"Stabilize login flow", "debugging", 0.78, 0.95},
      {"Tighten deploy script", "automation", 0.72, 0.70},
      {"Clarify onboarding guide", "docs", 0.58, 0.62},
      {"Shape plugin roadmap", "design", 0.68, 0.80},
  };

  Agent agent;

  std::cout << "agent starts small, chooses its own work, learns, and leaves light footprints\n";
  for (int step = 0; step < 12; ++step) {
    Target* target = agent.ChooseTarget(targets);
    if (target == nullptr) {
      break;
    }

    const bool success = agent.Work(*target);
    agent.PrintStep(*target, success);
  }

  agent.PrintSummary(targets);
  return 0;
}
