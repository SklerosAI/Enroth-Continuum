# MM6 BEAC renderer candidate contract — F4

Status, 2026-11-06: **proposal and blocker; no renderer or live SaveLoad change**. This follows the standalone `BEAC` codec and detached CPU image preparation in [the beacon save packet](MM6_F4_BEACON_VISUAL_SAVE.md). It does not establish a playable or reloadable MM6 save.

## Current source behavior

- `GraphicsImage::Create(RgbaImage)` in `src/Engine/Graphics/Image.cpp` stores CPU pixels and immediately calls `render->CreateTexture()`. `GraphicsImage::renderId()` also creates a texture on demand. The image destructor calls `releaseRenderId()` only while the global renderer exists.
- `OpenGLRenderer::CreateTexture()` in `src/Engine/Graphics/Renderer/OpenGLRenderer.cpp` uses an uninitialized local `GLuint`, then calls `glGenTextures`, `glBindTexture`, `glTexImage2D`, sets parameters and returns the ID. It has no local zero/error check or failed-upload cleanup. `TextureRenderId::isValid()` accepts every value except `-1`, including zero. A small local probe confirms that zero currently reports valid.
- `GL_Check_Errors()` is installed as the GLAD post-call callback during renderer initialization. It drains GL errors for warnings. Calling `glGetError()` after the texture function returns is therefore not a reliable creation result while that callback is active.
- `OpenGLRenderer::DeleteTexture()` appends to a `std::vector` for deletion after `swapBuffers()`. The append can allocate. `GraphicsImage` destruction can therefore invoke an allocation on a cleanup path, while the image destructor has no exception contract or rollback channel. The queue drains on a later buffer swap, and the renderer/context must remain alive through candidate disposal.
- `NullRenderer::CreateTexture()` returns ID 1 and its deletion is a no-op. Headless success alone cannot verify the OpenGL failure path. This is a shared renderer used by MM7, so a generic behavioral change needs an MM7 graphical regression gate.

## Proposed API and transaction order

1. Add an explicit checked texture-creation result at the renderer boundary, carrying a positive texture ID or a failure code. Initialize the local OpenGL ID to zero. Scope GL error capture to the texture calls through the existing GLAD callback or an equivalent single-thread mechanism; do not rely on a later `glGetError()` that the callback has already drained. Reject zero and any detected allocation/upload error, unbind, and delete a generated but unpublished ID before returning failure. The NullRenderer should return a deterministic positive success ID for headless transaction tests.
2. Add a controlled unpublished-candidate disposal path, or a bounded reservation of deferred-delete queue capacity before any candidate is created. Candidate failure and old-owner cleanup must be no-throw at the publication boundary. The normal deferred deletion policy for textures already referenced by render lists must remain intact. Do not switch all deletes to immediate `glDeleteTextures()` merely to avoid allocation.
3. Give `GraphicsImage` a checked constructor/factory that uses this result without changing the existing MM7 factory until its callers are audited. Keep the CPU `RgbaImage` in each candidate and hold all candidate images in detached Party state. Any `shared_ptr` control-block allocation happens before live mutation.
4. On the game/render thread, finish complete `.ec6` and all-owner validation, BEAC map/clock resolution, CPU reconstruction, checked GPU candidates, old-owner deletion capacity and rollback snapshot. Then transfer the fully prepared Party and other owners through proved no-throw swaps. Release old images while renderer/context and the safe deletion path are active. On any precommit failure, dispose only detached candidates and leave live owners unchanged.

## Required proof before opening BEAC

- Unit/fault-injection: zero ID, GL allocation/upload error, exception or allocation failure during the Nth of 20 candidates, and cleanup of all earlier candidates without touching the live Party. The checked result must not inherit or lose an unrelated GL error through the post-call callback.
- Renderer-backed: on the Mac OpenGL path, verify successful image appearance, failure diagnostics, and deferred deletion after buffer swap. Confirm old render-list references are not deleted early. Run the protected MM7 graphical load/UI baseline after any shared renderer change.
- Transaction: prove all-owner preflight and no-throw or rollback commit, image lifetime through the renderer, and save–quit–relaunch–load for MM6 beacons and thumbnail. The existing `KnownSaveFields` and active-beacon guards remain closed until then.

The narrow `GLuint = 0` and positive-ID checks would remove one false-success case, but would not detect every GL upload failure or make cleanup safe. Applying those generic changes alone would leave the BEAC publication contract open and would affect MM7; this lane makes no such partial renderer edit.
