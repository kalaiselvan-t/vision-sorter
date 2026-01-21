# Contributing to Franka Pick and Place

Thank you for your interest in contributing! This document provides guidelines for contributing to this project.

## Code of Conduct

- Be respectful and inclusive
- Focus on constructive feedback
- Help others learn and grow

## How to Contribute

### Reporting Bugs

If you find a bug, please open an issue with:
- Clear description of the problem
- Steps to reproduce
- Expected vs actual behavior
- Your environment (ROS version, OS, etc.)

### Suggesting Enhancements

Enhancement suggestions are welcome! Please open an issue with:
- Clear description of the feature
- Why it would be useful
- Potential implementation approach (optional)

### Pull Requests

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes
4. Follow the coding style (see below)
5. Test your changes
6. Commit with clear messages
7. Push to your fork
8. Open a Pull Request

## Coding Style

### C++
- Follow [ROS 2 C++ Style Guide](https://docs.ros.org/en/humble/The-ROS2-Project/Contributing/Code-Style-Language-Versions.html)
- Use `clang-format` with the provided `.clang-format` file
- Run `clang-tidy` for static analysis

### Python
- Follow [PEP 8](https://pep8.org/)
- Use type hints where appropriate
- Document functions with docstrings

### Commit Messages
- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- Reference issues and pull requests when relevant
- Follow conventional commits format:
  - `feat:` for new features
  - `fix:` for bug fixes
  - `docs:` for documentation
  - `refactor:` for code refactoring
  - `test:` for adding tests

## Testing

- Add unit tests for new functionality
- Ensure existing tests pass
- Test in simulation before submitting

## Documentation

- Update README.md if needed
- Add inline comments for complex logic
- Update CHANGELOG.md

## License

By contributing, you agree that your contributions will be licensed under the Apache 2.0 License.
