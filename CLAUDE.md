# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

59Music is a local music player with a C++ backend and Vue frontend, using SQLite for data storage. The project is currently in early stages — as of now, only `README.md` and `.gitignore` exist in the repository. The planned `frontend/` and `backend/` directories have not yet been created.

## Architecture

- **Frontend**: Vue (SPA)
- **Backend**: C++
- **Database**: SQLite

Planned project structure per `README.md`:
- `frontend/` — Vue application
- `backend/` — C++ source and CMake build

## Common Commands

### Frontend

```bash
cd frontend
npm install
npm run dev
```

### Backend

```bash
cd backend
mkdir build && cd build
cmake ..
make
```

## Notes

- The `.gitignore` already excludes standard C++ build artifacts (`build/`, `CMakeFiles/`, `*.exe`), Vue build output (`dist/`, `node_modules/`), SQLite files (`*.sqlite`, `*.sqlite3`), and IDE directories (`.vscode/`, `.idea/`).
- No source files, `package.json`, or `CMakeLists.txt` exist yet.
