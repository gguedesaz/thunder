#ifndef COMMANDBUFFER_H
#define COMMANDBUFFER_H

#include "engine.h"

class RenderTexture;
class Texture;
class Mesh;

class Camera;
class MaterialInstance;

typedef vector<RenderTexture *> TargetBuffer;

class NEXT_LIBRARY_EXPORT ICommandBuffer: public Object {
    A_REGISTER(ICommandBuffer, Object, System)

public:
    enum LayerTypes {
        DEFAULT     = (1<<0),
        RAYCAST     = (1<<1),
        SHADOWCAST  = (1<<2),
        LIGHT       = (1<<3),
        TRANSLUCENT = (1<<4),
        UI          = (1<<6)
    };

public:
    virtual void                clearRenderTarget           (bool clearColor = true, const Vector4 &color = Vector4(), bool clearDepth = true, float depth = 1.0f);

    virtual void                drawMesh                    (const Matrix4 &model, Mesh *mesh, uint32_t layer = ICommandBuffer::DEFAULT, MaterialInstance *material = nullptr);

    virtual void                drawMeshInstanced           (const Matrix4 *models, uint32_t count, Mesh *mesh, uint32_t layer = ICommandBuffer::DEFAULT, MaterialInstance *material = nullptr, bool particle = false);

    virtual void                setRenderTarget             (const TargetBuffer &target, RenderTexture *depth = nullptr);

    virtual void                setRenderTarget             (uint32_t target);

    virtual void                setColor                    (const Vector4 &color);

    virtual void                setScreenProjection         ();

    virtual void                resetViewProjection         ();

    virtual void                setViewProjection           (const Matrix4 &view, const Matrix4 &projection);

    virtual void                setGlobalValue              (const char *name, const Variant &value);

    virtual void                setGlobalTexture            (const char *name, Texture *value);

    virtual void                setViewport                 (int32_t x, int32_t y, int32_t width, int32_t height);

    virtual void                enableScissor               (int32_t x, int32_t y, int32_t width, int32_t height);

    virtual void                disableScissor              ();

    virtual Matrix4             projection                  () const;

    virtual Matrix4             modelView                   () const;

    virtual Texture            *texture                     (const char *name) const;

    static Vector4              idToColor                   (uint32_t id);

    static bool                 isInited                    ();

    static void                 setInited                   ();

};

#endif // COMMANDBUFFER_H
