# team_git

A tiny C++ simulation of an agent that:

- chooses its own next target
- learns from failed attempts
- improves its skills over time
- leaves light footprints behind as short progress markers

## Build

```bash
g++ -std=c++17 /home/runner/work/team_git/team_git/main.cpp /home/runner/work/team_git/team_git/func.cpp /home/runner/work/team_git/team_git/kernel.cpp -o /tmp/team_git_agent
```

## Run

```bash
/tmp/team_git_agent
```
