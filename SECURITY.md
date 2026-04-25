# Security Policy

## Supported versions

Only the latest release on `main` receives security fixes.

## Reporting a vulnerability

If you find a security issue — for example in the CSV parser, the config loader, or any other input-handling path — please **do not open a public GitHub issue**.

Instead, email the maintainer directly or open a [GitHub private security advisory](https://docs.github.com/en/code-security/security-advisories/guidance-on-reporting-and-writing/privately-reporting-a-security-vulnerability).

Include:

- A description of the issue and its potential impact
- Steps to reproduce or a proof-of-concept
- The version of Algo-Catalyst you tested against

You will receive a response within 72 hours. Once a fix is ready, a patched release will be published and the issue will be publicly disclosed in `CHANGELOG.md`.

## Scope

This project is a backtesting engine intended for research and educational use. It does not handle real money, live trading connections, or user credentials, so the attack surface is limited to:

- Malformed CSV input to the tick loader
- Malformed JSON input to the config loader
- Unsafe file paths passed via CLI flags

We still take these seriously and will fix any reproducible issues promptly.
