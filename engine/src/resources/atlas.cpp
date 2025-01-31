#include "atlas.h"

#include "engine.h"
#include "texture.h"

#include <cstring>

typedef deque<Texture *>  Textures;

PackNode::PackNode() :
        fill(false),
        x(0),
        y(0),
        w(1),
        h(1),
        parent(nullptr) {
    child[0] = nullptr;
    child[1] = nullptr;
}

PackNode::~PackNode() {
    delete child[0];
    delete child[1];
}

PackNode *PackNode::insert(int32_t width, int32_t height) {
    PROFILE_FUNCTION();

    if(child[0]) {
        PackNode *node = child[0]->insert(width, height);
        if(node) {
            return node;
        }
        return child[1]->insert(width, height);
    }

    if(fill || w < width || h < height) {
        return nullptr;
    }

    if(w == width && h == height) {
        return this;
    }
    // Request is smaller then node start splitting
    int32_t sw = w - width;
    int32_t sh = h - height;

    child[0] = new PackNode;
    child[0]->parent = this;
    child[1] = new PackNode;
    child[1]->parent = this;

    if(sw > sh) { // Horizontal
        child[0]->x = x;
        child[0]->y = y;
        child[0]->w = width;
        child[0]->h = h;

        child[1]->x = x + width;
        child[1]->y = y;
        child[1]->w = sw;
        child[1]->h = h;
    } else { // Vertical
        child[0]->x = x;
        child[0]->y = y;
        child[0]->w = w;
        child[0]->h = height;

        child[1]->x = x;
        child[1]->y = y + height;
        child[1]->w = w;
        child[1]->h = sh;
    }

    return child[0]->insert(width, height);
}

bool PackNode::clean () {
    PROFILE_FUNCTION();

    if(child[0] && child[0]->clean()) {
        child[0] = nullptr;
        delete child[1];
        child[1] = nullptr;
    }

    if(child[0] == nullptr && child[1] == nullptr && !fill && parent) {
        delete this;
        return true;
    }
    return false;
}

class AtlasPrivate {
public:
    AtlasPrivate() {
        m_pTexture = nullptr;

        m_pRoot = new PackNode;
    }
    Vector4Vector m_Elements;

    Texture *m_pTexture;

    Textures m_Sources;

    PackNode *m_pRoot;
};

Atlas::Atlas() :
        p_ptr(new AtlasPrivate) {

    p_ptr->m_pTexture  = Engine::objectCreate<Texture>("", this);
    p_ptr->m_pTexture->setFiltering(Texture::Bilinear);
    resize(1024, 1024);
}

Atlas::~Atlas() {
    PROFILE_FUNCTION();

    clear();

    delete p_ptr;
}

void Atlas::clear() {
    PROFILE_FUNCTION();

    p_ptr->m_Sources.clear();
    p_ptr->m_Elements.clear();

}

uint32_t Atlas::addElement(Texture *texture) {
    PROFILE_FUNCTION();

    p_ptr->m_Sources.push_back(texture);
    return (p_ptr->m_Sources.size() - 1);
}

void Atlas::pack(uint8_t padding) {
    PROFILE_FUNCTION();

    p_ptr->m_Elements.clear();
    for(auto it : p_ptr->m_Sources) {
        int32_t width  = (it->width() + padding);
        int32_t height = (it->height() + padding);

        PackNode *n = p_ptr->m_pRoot->insert(width, height);
        if(n) {
            n->fill = true;
            int32_t w = n->w - padding;
            int32_t h = n->h - padding;

            Vector4 res;
            res.x = n->x / static_cast<float>(p_ptr->m_pRoot->w);
            res.y = n->y / static_cast<float>(p_ptr->m_pRoot->h);
            res.z = res.x + w / static_cast<float>(p_ptr->m_pRoot->w);
            res.w = res.y + h / static_cast<float>(p_ptr->m_pRoot->h);

            uint8_t *src = it->surface(0)[0];
            uint8_t *dst = p_ptr->m_pTexture->surface(0)[0];
            for(int32_t y = 0; y < h; y++) {
                memcpy(&dst[(y + n->y) * p_ptr->m_pRoot->w + n->x], &src[y * w], w);
            }

            p_ptr->m_Elements.push_back(res);
        } else {
            resize(p_ptr->m_pRoot->w * 2, p_ptr->m_pRoot->h * 2);
            pack(padding);
        }
    }

    p_ptr->m_pTexture->setDirty();
}

void Atlas::resize(int32_t width, int32_t height) {
    PROFILE_FUNCTION();

    if(p_ptr->m_pRoot) {
        delete p_ptr->m_pRoot;
        p_ptr->m_pRoot = new PackNode;
    }
    p_ptr->m_pRoot->w = width;
    p_ptr->m_pRoot->h = height;

    p_ptr->m_pTexture->resize(width, height);
}

Vector2Vector Atlas::shape(uint32_t index) const {
    PROFILE_FUNCTION();

    if(index < p_ptr->m_Sources.size()) {
        return p_ptr->m_Sources[index]->shape();
    }
    return Vector2Vector();
}

Vector4 Atlas::uv(uint32_t index) const {
    PROFILE_FUNCTION();

    if(index < p_ptr->m_Elements.size()) {
        return p_ptr->m_Elements[index];
    }
    return Vector4();
}

Texture *Atlas::texture() const {
    PROFILE_FUNCTION();

    return p_ptr->m_pTexture;
}
