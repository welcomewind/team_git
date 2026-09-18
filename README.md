# team_git

A small C++ agent that performs bounded live local actions instead of only
simulating them.

It can:

- choose its next objective based on expected impact and readiness
- form a small local team when that appears more efficient and profitable
- learn from failed attempts and retry with improved skill
- create a local self-custody wallet artifact for accumulated gains
- leave short operational footprints in runtime logs

At runtime it writes its live artifacts under your system temporary directory in
`team_git_live_agent/`, including:

- `status.log`
- `team_roster.txt`
- `repo_inventory.txt`
- `footprints.log`
- `self_custody_wallet.txt`

## Build

```bash
g++ -std=c++17 main.cpp func.cpp kernel.cpp -o team_git_agent
```

## Run

```bash
./team_git_agent
```

You can also pass a repository path explicitly:

```bash
./team_git_agent /absolute/path/to/repo
```
