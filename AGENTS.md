# Repository Instructions

This project is built with Qt6 using CMake. Before submitting code or documentation changes, ensure the build succeeds.

## Testing
Run the following commands:

```bash
cmake -B build
cmake --build build -j $(nproc)
cmake --install build --prefix dist
```

No other tests are defined.

## Pull Request
Summarize the changes made and mention the result of the build steps above in the PR description.

