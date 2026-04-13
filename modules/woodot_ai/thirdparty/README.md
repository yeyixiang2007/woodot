# Woodot AI Third-Party Dependencies

## llama.cpp

- Upstream: `https://github.com/ggml-org/llama.cpp`
- Vendored path: `modules/woodot_ai/thirdparty/llama.cpp`
- Pinned commit: `873c825611d9cb76427931b5e74642bade4853dd`

Vendoring rules:

- Keep upstream sources inside the `llama.cpp/` subtree.
- Do not expose upstream headers directly through Woodot public APIs.
- Prefer keeping Woodot-specific integration in `modules/woodot_ai/backends/` and build glue in `modules/woodot_ai/SCsub`.
- If local patches become necessary, document them in this directory before rebasing or upgrading the vendored copy.
