#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

enum class ActionKind {
  kPrepareWorkspace,
  kCreateTeam,
  kCaptureInventory,
  kCreateWallet,
  kLeaveFootprint,
};

struct Objective {
  std::string name;
  std::string domain;
  ActionKind kind;
  double difficulty;
  double impact;
  double reward;
  int attempts = 0;
  bool completed = false;
};

class Workspace {
 public:
  explicit Workspace(fs::path repo_root)
      : repo_root_(std::move(repo_root)),
        base_path_(fs::temp_directory_path() / "team_git_live_agent"),
        wallet_path_(base_path_ / "self_custody_wallet.txt"),
        wallet_secret_path_(base_path_ / "self_custody_wallet.private") {}

  bool ready() const { return fs::exists(base_path_); }
  bool wallet_created() const { return wallet_created_; }
  bool team_created() const { return team_created_; }
  const fs::path& base_path() const { return base_path_; }
  const fs::path& wallet_path() const { return wallet_path_; }
  const fs::path& wallet_secret_path() const { return wallet_secret_path_; }
  double balance() const { return wallet_balance_; }
  static std::string SecretAccessCommand() {
    return "./team_git_agent --show-wallet-secret";
  }

  std::string PrepareWorkspace() {
    fs::create_directories(base_path_);
    AppendLine(base_path_ / "status.log", Timestamp() + " workspace ready");
    return "workspace ready at " + base_path_.string();
  }

  std::string CreateTeam() {
    if (!ready()) {
      return "workspace not ready";
    }

    team_members_ = {"Scout", "Builder", "Auditor"};
    std::ofstream output(base_path_ / "team_roster.txt");
    output << "team=adaptive_ops\n";
    output << "purpose=improve efficiency and profitability\n";
    for (const auto& member : team_members_) {
      output << "member=" << member << '\n';
    }

    team_created_ = true;
    AppendLine(base_path_ / "status.log",
               Timestamp() + " created adaptive_ops team");
    return "team formed at " + (base_path_ / "team_roster.txt").string();
  }

  std::string CaptureInventory() {
    if (!ready()) {
      return "workspace not ready";
    }

    std::vector<std::string> entries;
    for (const auto& entry : fs::directory_iterator(repo_root_)) {
      entries.push_back(entry.path().filename().string());
    }

    std::sort(entries.begin(), entries.end());
    std::ofstream output(base_path_ / "repo_inventory.txt");
    for (const auto& name : entries) {
      output << name << '\n';
    }

    AppendLine(base_path_ / "status.log",
               Timestamp() + " captured " + std::to_string(entries.size()) +
                   " repo entries");
    return "captured repo inventory";
  }

  std::string CreateWallet(double opening_balance) {
    if (!ready()) {
      return "workspace not ready";
    }

    if (!wallet_created_) {
      wallet_secret_ = RandomHex(32);
      wallet_address_ = "wallet_" + RandomHex(12);
      wallet_created_ = true;
    }

    wallet_balance_ += opening_balance;
    WriteWalletSummary();
    WriteWalletSecret();
    AppendLine(base_path_ / "status.log",
               Timestamp() + " wallet balance " + Format(wallet_balance_));
    return "wallet ready at " + wallet_path_.string();
  }

  void BankGain(double gain) {
    if (!wallet_created_ || gain <= 0.0) {
      return;
    }

    wallet_balance_ += gain;
    WriteWalletSummary();
    WriteWalletSecret();
    AppendLine(base_path_ / "status.log",
               Timestamp() + " credited gain " + Format(gain));
  }

  std::string LeaveFootprint(const std::string& note) {
    if (!ready()) {
      return "workspace not ready";
    }

    AppendLine(base_path_ / "footprints.log", Timestamp() + " " + note);
    return "footprint recorded";
  }

 private:
  static std::string Timestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t raw_time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_time{};
#if defined(_WIN32)
    gmtime_s(&utc_time, &raw_time);
#else
    gmtime_r(&raw_time, &utc_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
  }

  static std::string RandomHex(std::size_t bytes) {
    static std::random_device device;
    static std::mt19937 engine(device());
    static std::uniform_int_distribution<int> dist(0, 255);

    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes; ++i) {
      stream << std::setw(2) << dist(engine);
    }
    return stream.str();
  }

  static std::string Format(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
  }

  static void AppendLine(const fs::path& path, const std::string& line) {
    std::ofstream output(path, std::ios::app);
    output << line << '\n';
  }

  void WriteWalletSummary() const {
    std::ofstream output(wallet_path_);
    output << "mode=self-custody\n";
    output << "network=local\n";
    output << "address=" << wallet_address_ << '\n';
    output << "balance=" << Format(wallet_balance_) << '\n';
    output << "secret_status=stored_local_only\n";
    output << "secret_access_command=" << SecretAccessCommand() << '\n';
  }

  void WriteWalletSecret() const {
    std::ofstream output(wallet_secret_path_);
    output << "mode=self-custody\n";
    output << "network=local\n";
    output << "address=" << wallet_address_ << '\n';
    output << "private_key=" << wallet_secret_ << '\n';

    std::error_code error;
    fs::permissions(wallet_secret_path_,
                    fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace, error);
  }

  fs::path repo_root_;
  fs::path base_path_;
  fs::path wallet_path_;
  fs::path wallet_secret_path_;
  bool team_created_ = false;
  bool wallet_created_ = false;
  double wallet_balance_ = 0.0;
  std::string wallet_address_;
  std::string wallet_secret_;
  std::vector<std::string> team_members_;
};

class Agent {
 public:
  Agent()
      : skills_({{"filesystem", 0.62},
                 {"planning", 0.54},
                 {"wallet", 0.38},
                 {"leadership", 0.42}}) {}

  Objective* ChooseObjective(std::vector<Objective>& objectives,
                             const Workspace& workspace) {
    Objective* best = nullptr;
    double best_score = -1.0;
    const double team_opportunity = EvaluateTeamOpportunity(objectives);

    for (auto& objective : objectives) {
      if (objective.completed) {
        continue;
      }

      if (!workspace.ready() &&
          objective.kind != ActionKind::kPrepareWorkspace) {
        continue;
      }

      const double skill = skills_[objective.domain];
      const double curiosity_bonus = objective.attempts == 0 ? 0.14 : 0.0;
      const double persistence_bonus = objective.attempts * 0.10;
      const double team_bonus =
          objective.kind == ActionKind::kCreateTeam ? team_opportunity : 0.0;
      const double score = (objective.impact * 1.25) + skill + curiosity_bonus +
                           persistence_bonus + team_bonus - objective.difficulty;

      if (score > best_score) {
        best_score = score;
        best = &objective;
      }
    }

    return best;
  }

  bool Execute(Objective& objective, Workspace& workspace) {
    ++objective.attempts;

    std::string detail;
    bool success = false;

    if (objective.kind != ActionKind::kPrepareWorkspace && !workspace.ready()) {
      detail = "workspace missing";
    } else {
      const double readiness = ReadinessFor(objective, workspace);
      if (readiness < objective.difficulty) {
        detail = "judgment improved after a miss";
      } else {
        success = Perform(objective, workspace, detail);
      }
    }

    if (success) {
      objective.completed = true;
      skills_[objective.domain] =
          std::min(1.0, skills_[objective.domain] + 0.11);
      footprints_.push_back("👣 " + objective.name);
      Trim(footprints_);
      if (objective.reward > 0.0) {
        unbanked_gains_ += objective.reward;
        workspace.BankGain(unbanked_gains_);
        if (workspace.wallet_created()) {
          unbanked_gains_ = 0.0;
        }
      }
    } else {
      skills_[objective.domain] =
          std::min(1.0, skills_[objective.domain] + 0.20);
      std::ostringstream lesson;
      lesson << "missed " << objective.name << ", " << detail << ", "
             << objective.domain << " skill " << Format(skills_[objective.domain]);
      lessons_.push_back(lesson.str());
      Trim(lessons_);
    }

    last_detail_ = detail;
    return success;
  }

  void PrintStep(const Objective& objective, bool success) const {
    std::cout << "- chose: " << objective.name << " [" << objective.domain << "] -> "
              << (success ? "completed live action" : "stumbled, learned, retry later")
              << " (" << last_detail_ << ")\n";
  }

  void PrintSummary(const std::vector<Objective>& objectives,
                    const Workspace& workspace) const {
    const auto completed = std::count_if(
        objectives.begin(), objectives.end(),
        [](const Objective& objective) { return objective.completed; });

    std::cout << "\nsummary\n";
    std::cout << "completed " << completed << " of " << objectives.size()
              << " objectives\n";
    std::cout << "workspace: " << workspace.base_path() << '\n';

    if (!lessons_.empty()) {
      std::cout << "recent lessons\n";
      for (const auto& lesson : lessons_) {
        std::cout << "  - " << lesson << '\n';
      }
    }

    if (workspace.wallet_created()) {
      std::cout << "wallet: " << workspace.wallet_path() << '\n';
      std::cout << "wallet balance: " << Format(workspace.balance()) << '\n';
      std::cout << "wallet secret access: run `"
                << Workspace::SecretAccessCommand() << "` locally\n";
    } else if (unbanked_gains_ > 0.0) {
      std::cout << "unbanked gains: " << Format(unbanked_gains_) << '\n';
    }

    if (workspace.team_created()) {
      std::cout << "team support: adaptive_ops active\n";
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
    constexpr std::size_t kMaxEntries = 5;
    if (entries.size() > kMaxEntries) {
      entries.erase(entries.begin(), entries.begin() + (entries.size() - kMaxEntries));
    }
  }

  static std::string Format(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
  }

  bool Perform(const Objective& objective, Workspace& workspace,
               std::string& detail) {
    switch (objective.kind) {
      case ActionKind::kPrepareWorkspace:
        detail = workspace.PrepareWorkspace();
        return true;
      case ActionKind::kCreateTeam:
        detail = workspace.CreateTeam();
        return detail != "workspace not ready";
      case ActionKind::kCaptureInventory:
        detail = workspace.CaptureInventory();
        return detail != "workspace not ready";
      case ActionKind::kCreateWallet:
        detail = workspace.CreateWallet(unbanked_gains_);
        if (workspace.wallet_created()) {
          unbanked_gains_ = 0.0;
        }
        return detail != "workspace not ready";
      case ActionKind::kLeaveFootprint:
        detail = workspace.LeaveFootprint("completed " + objective.name);
        return detail != "workspace not ready";
    }

    detail = "unknown objective";
    return false;
  }

  double EvaluateTeamOpportunity(const std::vector<Objective>& objectives) const {
    double remaining_profit = 0.0;
    int demanding_objectives = 0;

    for (const auto& objective : objectives) {
      if (objective.completed || objective.kind == ActionKind::kCreateTeam) {
        continue;
      }

      remaining_profit += objective.reward;
      if (objective.difficulty > 0.70) {
        ++demanding_objectives;
      }
    }

    return (remaining_profit * 0.18) + (demanding_objectives * 0.14);
  }

  double ReadinessFor(const Objective& objective, const Workspace& workspace) const {
    double readiness = skills_.at(objective.domain) + (objective.attempts - 1) * 0.18;
    if (workspace.team_created() && objective.kind != ActionKind::kCreateTeam) {
      readiness += 0.16;
    }
    return readiness;
  }

  std::unordered_map<std::string, double> skills_;
  std::vector<std::string> lessons_;
  std::vector<std::string> footprints_;
  double unbanked_gains_ = 0.0;
  std::string last_detail_;
};

int ShowWalletSecret() {
  const fs::path base_path = fs::temp_directory_path() / "team_git_live_agent";
  const fs::path secret_path = base_path / "self_custody_wallet.private";

  if (!fs::exists(secret_path)) {
    std::cerr << "no local wallet secret found at " << secret_path << '\n';
    return 1;
  }

  std::ifstream input(secret_path);
  std::cout << input.rdbuf();
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc > 1 && std::string(argv[1]) == "--show-wallet-secret") {
    return ShowWalletSecret();
  }

  const fs::path repo_root =
      argc > 1 ? fs::path(argv[1]) : fs::current_path();

  std::vector<Objective> objectives = {
      {"Prepare local workspace", "planning", ActionKind::kPrepareWorkspace,
       0.45, 0.95, 0.00},
      {"Create adaptive team", "leadership", ActionKind::kCreateTeam, 0.69, 0.90,
       0.65},
      {"Capture repository inventory", "filesystem",
       ActionKind::kCaptureInventory, 0.78, 0.88, 1.40},
      {"Create self-custody wallet", "wallet", ActionKind::kCreateWallet,
       0.76, 0.84, 0.00},
      {"Leave light operational footprint", "filesystem",
       ActionKind::kLeaveFootprint, 0.73, 0.80, 0.45},
  };

  Workspace workspace(repo_root);
  Agent agent;

  std::cout << "agent operates on the most efficient local action it can execute live\n";
  std::cout << "it may form a team when that looks more efficient and profitable\n";
  std::cout << "repo root: " << repo_root << "\n\n";

  for (int step = 0; step < 12; ++step) {
    Objective* objective = agent.ChooseObjective(objectives, workspace);
    if (objective == nullptr) {
      break;
    }

    const bool success = agent.Execute(*objective, workspace);
    agent.PrintStep(*objective, success);
  }

  agent.PrintSummary(objectives, workspace);
  return 0;
}
