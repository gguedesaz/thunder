#include "postprocess/ambientocclusion.h"

#include "engine.h"

#include "pipeline.h"
#include "material.h"

#include "amath.h"

AmbientOcclusion::AmbientOcclusion() {
    Material *material = Engine::loadResource<Material>(".embedded/AmbientOcclusion.mtl");
    if(material) {
        m_pMaterial = material->createInstance();
    }

    m_pSamplesKernel = new Vector3[128];
    for(int i = 0; i < 128; i++) {
        m_pSamplesKernel[i].x = RANGE(-1.0f, 1.0f);
        m_pSamplesKernel[i].z = RANGE(-1.0f, 1.0f);
        m_pSamplesKernel[i].y = RANGE( 0.0f, 1.0f);

        m_pSamplesKernel[i].normalize();

        float scale = (float)i / 128.0f;
        scale = MIX(0.1f, 1.0f, scale * scale);
        m_pSamplesKernel[i] *= scale;
    }
}

AmbientOcclusion::~AmbientOcclusion() {
    delete m_pSamplesKernel;
}
