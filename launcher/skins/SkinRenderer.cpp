/* Copyright 2025 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "SkinRenderer.h"
#include "SkinTypes.h"
#include "TextureMappings.h"
#include <algorithm>
#include <array>

#include <QVector4D>

namespace Skins {

struct VertexBg {
    GLfloat m_position[2];
};

static void RenderQuad(
    RenderContext& context,
    const QVector3D& p0,
    const QVector3D& p1,
    const QVector3D& p2,
    const QVector3D& p3,
    const Rectangle& mapping,
    const Texture texture,
    const bool transparency,
    const float wPixel,
    const float hPixel
) {
    GLint textureId = GLint(texture);

    float left, right, bottom, top;
    if(mapping.flipX)
    {
        right = mapping.x * wPixel;
        left = (mapping.x + mapping.w) * wPixel;
    }
    else
    {
        left = mapping.x * wPixel;
        right = (mapping.x + mapping.w) * wPixel;
    }
    if(mapping.flipY)
    {
        bottom = mapping.y * hPixel;
        top = (mapping.y + mapping.h) * hPixel;
    }
    else
    {
        top = mapping.y * hPixel;
        bottom = (mapping.y + mapping.h) * hPixel;
    }

    context.m_vertexBuffer.push_back(Vertex(p0, left, bottom, textureId, transparency));
    context.m_vertexBuffer.push_back(Vertex(p1, right, bottom, textureId, transparency));
    context.m_vertexBuffer.push_back(Vertex(p2, right, top, textureId, transparency));
    context.m_vertexBuffer.push_back(Vertex(p3, left, top, textureId, transparency));

    context.m_elementBuffer.push_back(context.m_elementStartIndex + 0);
    context.m_elementBuffer.push_back(context.m_elementStartIndex + 1);
    context.m_elementBuffer.push_back(context.m_elementStartIndex + 3);

    context.m_elementBuffer.push_back(context.m_elementStartIndex + 1);
    context.m_elementBuffer.push_back(context.m_elementStartIndex + 2);
    context.m_elementBuffer.push_back(context.m_elementStartIndex + 3);

    context.m_elementStartIndex += 4;
}

void RenderBox(RenderContext& context, float width, float height, float depth, const TextureMapping& mapping, QMatrix4x4 transform)
{
    std::array<QVector3D, 8> vertices = {
        transform * QVector3D(-0.5f*width, -0.5f*height,  0.5f*depth),
        transform * QVector3D( 0.5f*width, -0.5f*height,  0.5f*depth),
        transform * QVector3D( 0.5f*width,  0.5f*height,  0.5f*depth),
        transform * QVector3D(-0.5f*width,  0.5f*height,  0.5f*depth),

        transform * QVector3D(-0.5f*width, -0.5f*height, -0.5f*depth),
        transform * QVector3D( 0.5f*width, -0.5f*height, -0.5f*depth),
        transform * QVector3D( 0.5f*width,  0.5f*height, -0.5f*depth),
        transform * QVector3D(-0.5f*width,  0.5f*height, -0.5f*depth),
    };

    float wPixel;
    float hPixel;

    if(mapping.material == Texture::Skin)
    {
        wPixel = 1.0 / float(context.m_skinTexture->width());
        hPixel = 1.0 / float(context.m_skinTexture->height());
    }
    else
    {
        if(context.m_capeTexture)
        {
            wPixel = 1.0 / float(context.m_capeTexture->width());
            hPixel = 1.0 / float(context.m_capeTexture->height());
        }
        else
        {
            wPixel = 0.1;
            hPixel = 0.1;
        }
    }

    RenderQuad(
        context,
        vertices[0],
        vertices[1],
        vertices[2],
        vertices[3],
        mapping.front,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );

    RenderQuad(
        context,
        vertices[1],
        vertices[5],
        vertices[6],
        vertices[2],
        mapping.left,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );

    RenderQuad(
        context,
        vertices[5],
        vertices[4],
        vertices[7],
        vertices[6],
        mapping.back,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );

    RenderQuad(
        context,
        vertices[4],
        vertices[0],
        vertices[3],
        vertices[7],
        mapping.right,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );

    RenderQuad(
        context,
        vertices[4],
        vertices[5],
        vertices[1],
        vertices[0],
        mapping.bottom,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );

    RenderQuad(
        context,
        vertices[3],
        vertices[2],
        vertices[6],
        vertices[7],
        mapping.top,
        mapping.material,
        mapping.transparent,
        wPixel,
        hPixel
    );
}

void RenderSkin(RenderContext& context, int version, Model model)
{
    // TODO: deadmau5 ears
    QMatrix4x4 torsoPosition;
    QMatrix4x4 headPosition;
    QMatrix4x4 leftArmPosition;
    QMatrix4x4 rightArmPosition;
    QMatrix4x4 leftLegPosition;
    QMatrix4x4 rightLegPosition;
    torsoPosition.translate(0, 2, 0);
    headPosition.translate(0, 12, 0);
    leftLegPosition.translate(2, -10, 0);
    rightLegPosition.translate(-2, -10, 0);

    QMatrix4x4 capeTransform;
    capeTransform.translate(0, 8, -2);
    capeTransform.rotate(180, 0, 1, 0);
    capeTransform.rotate(-10, 1, 0, 0);
    capeTransform.translate(0, -8, 0.5);

    if(context.m_capeTexture)
    {
        auto capeScale = context.m_capeTexture->height() >= 34 ? 0 : 1;
        RenderBox(context, 10, 16, 1, capeLayout[capeScale], capeTransform);
    }

    if(model == Model::Classic)
    {
        leftArmPosition.translate(6, 2, 0);
        rightArmPosition.translate(-6, 2, 0);

        RenderBox(context, 8, 8, 8, head, headPosition);
        RenderBox(context, 8, 12, 4, torso, torsoPosition);
        if(version == 1)
        {
            RenderBox(context, 4, 12, 4, left_leg, leftLegPosition);
            RenderBox(context, 4, 12, 4, right_leg, rightLegPosition);
            RenderBox(context, 4, 12, 4, left_arm_classic, leftArmPosition);
            RenderBox(context, 4, 12, 4, right_arm_classic, rightArmPosition);
        }
        else
        {
            RenderBox(context, 4, 12, 4, left_leg_old, leftLegPosition);
            RenderBox(context, 4, 12, 4, right_leg, rightLegPosition);
            RenderBox(context, 4, 12, 4, left_arm_old_classic, leftArmPosition);
            RenderBox(context, 4, 12, 4, right_arm_old_classic, rightArmPosition);
        }

        RenderBox(context, 9, 9, 9, head_cover, headPosition);
        if(version == 1)
        {
            RenderBox(context, 9, 13, 5, torso_cover, torsoPosition);
            RenderBox(context, 5, 13, 5.03, left_arm_cover_classic, leftArmPosition);
            RenderBox(context, 5, 13, 5.03, right_arm_cover_classic, rightArmPosition);
            RenderBox(context, 5, 13, 5.01, left_leg_cover, leftLegPosition);
            RenderBox(context, 5, 13, 5.02, right_leg_cover, rightLegPosition);
        }
    }
    else
    {
        leftArmPosition.translate(5.5, 2, 0);
        rightArmPosition.translate(-5.5, 2, 0);

        RenderBox(context, 8, 8, 8, head, headPosition);
        RenderBox(context, 8, 12, 4, torso, torsoPosition);
        if(version == 1)
        {
            RenderBox(context, 4, 12, 4, left_leg, leftLegPosition);
            RenderBox(context, 4, 12, 4, right_leg, rightLegPosition);
            RenderBox(context, 3, 12, 4, left_arm_slim, leftArmPosition);
            RenderBox(context, 3, 12, 4, right_arm_slim, rightArmPosition);
        }
        else
        {
            RenderBox(context, 4, 12, 4, left_leg_old, leftLegPosition);
            RenderBox(context, 4, 12, 4, right_leg, rightLegPosition);
            RenderBox(context, 3, 12, 4, left_arm_old_slim, leftArmPosition);
            RenderBox(context, 3, 12, 4, right_arm_old_slim, rightArmPosition);
        }

        RenderBox(context, 9, 9, 9, head_cover, headPosition);
        if(version == 1)
        {
            RenderBox(context, 9, 13, 5, torso_cover, torsoPosition);
            RenderBox(context, 4, 13, 5.03, left_arm_cover_slim, leftArmPosition);
            RenderBox(context, 4, 13, 5.03, right_arm_cover_slim, rightArmPosition);
            RenderBox(context, 5, 13, 5.01, left_leg_cover, leftLegPosition);
            RenderBox(context, 5, 13, 5.02, right_leg_cover, rightLegPosition);
        }
    }
}


RenderContext::RenderContext(QOpenGLFunctions& GL, QObject* parent) : QObject(parent), GL(GL)
{
    m_skinShader = new QOpenGLShaderProgram(this);
    m_vao = new QOpenGLVertexArrayObject(this);
    m_ebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);

    m_bgShader = new QOpenGLShaderProgram(this);
    m_bgvao = new QOpenGLVertexArrayObject(this);
    m_bgebo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_bgvbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
}

void RenderContext::init()
{
    GL.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_uniforms.push_back(Skins::Uniform("worldToView"));
    m_uniforms.push_back(Skins::Uniform("skinTexture"));
    m_uniforms.push_back(Skins::Uniform("capeTexture"));
    m_uniforms.push_back(Skins::Uniform("renderPass"));
    initShader(m_skinShader, {"position", "texcoords", "texnr", "transparency"}, m_uniforms, ":/skins/shaders/skin.vert", ":/skins/shaders/skin.frag");

    m_bgUniforms.push_back(Skins::Uniform("baseColor"));
    m_bgUniforms.push_back(Skins::Uniform("alternateColor"));
    m_bgUniforms.push_back(Skins::Uniform("fDevicePixelSize"));
    initShader(m_bgShader, {"position"}, m_bgUniforms, ":/skins/shaders/bg.vert", ":/skins/shaders/bg.frag");

    GL.glEnable(GL_BLEND);
    GL.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_vao->create();
    m_vao->bind();
    {
        m_vbo->create();
        m_vbo->bind();
        m_vbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

        m_ebo->create();
        m_ebo->bind();
        m_ebo->setUsagePattern(QOpenGLBuffer::StaticDraw);

        m_skinShader->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, m_position), 3, sizeof(Vertex));
        m_skinShader->enableAttributeArray(0);
        m_skinShader->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, m_uv), 2, sizeof(Vertex));
        m_skinShader->enableAttributeArray(1);
        m_skinShader->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, m_texture), 1, sizeof(Vertex));
        m_skinShader->enableAttributeArray(2);
        m_skinShader->setAttributeBuffer(3, GL_FLOAT, offsetof(Vertex, m_transparency), 1, sizeof(Vertex));
        m_skinShader->enableAttributeArray(3);

        // NOTE: this crashes
        //m_ebo->release();
        m_vbo->release();
    }
    m_vao->release();

    m_bgvao->create();
    m_bgvao->bind();
    {
        m_bgvbo->create();
        m_bgvbo->bind();
        m_bgvbo->setUsagePattern(QOpenGLBuffer::StaticDraw);

        m_bgebo->create();
        m_bgebo->bind();
        m_bgebo->setUsagePattern(QOpenGLBuffer::StaticDraw);

        m_bgShader->setAttributeBuffer(0, GL_FLOAT, 0, 2, sizeof(VertexBg));
        m_bgShader->enableAttributeArray(0);

        // NOTE: this crashes
        //m_bgebo->release();
        m_bgvbo->release();
    }
    m_bgvao->release();

    std::array<VertexBg, 4> backQuad = {
        VertexBg{-1.0, -1.0f},
        VertexBg{ 1.0, -1.0f},
        VertexBg{ 1.0,  1.0f},
        VertexBg{-1.0,  1.0f}
    };
    std::array<GLuint, 6> backQuadElements = { 0, 1, 3, 1, 2, 3 };

    m_bgvbo->bind();
    m_bgvbo->allocate(backQuad.data(), backQuad.size()*sizeof(VertexBg));
    m_bgvbo->release();

    m_bgebo->bind();
    m_bgebo->allocate(backQuadElements.data(), backQuadElements.size()*sizeof(GLuint));
    m_bgebo->release();
}

void RenderContext::deinit()
{
    // TODO: do something here.
}

bool RenderContext::initShader(QOpenGLShaderProgram* program, const QStringList& attributes, QList<Uniform>& uniforms, const QString& vertexShaderPath, const QString& fragmentShaderPath)
{
    // read the shader programs from the resource
    if (!program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShaderPath))
    {
        qCritical() << "Error compiling vertex shader " << vertexShaderPath << ": " << program->log();
        return false;
    }

    if (!program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShaderPath))
    {
        qCritical() << "Error compiling fragment shader " << fragmentShaderPath << ": " << program->log();
        return false;
    }

    // The attribute buffers are set up by index, so pin the indexes. GLSL 1.50 has no layout(location) for inputs.
    for (int i = 0; i < attributes.size(); i++)
    {
        program->bindAttributeLocation(attributes[i], i);
    }

    if (!program->link())
    {
        qCritical() << "Shader linker error: " << program->log();
        return false;
    }

    for (Uniform& uniform : uniforms) {
        uniform.id = program->uniformLocation(uniform.name);
        if (uniform.id == -1)
        {
            qCritical() << "Error retrieving uniform ID for uniform " << uniform.name;
            return false;
        }
    }
    return true;
}

void Skins::RenderContext::setTextures(const QImage& skin, const QImage& cape)
{
    m_skinShader->bind();
    if(m_skinTexture)
    {
        m_skinTexture->destroy();
        delete m_skinTexture;
        m_skinTexture = nullptr;
    }
    if(m_capeTexture)
    {
        m_capeTexture->destroy();
        delete m_capeTexture;
        m_capeTexture = nullptr;
    }

    m_skinTexture = new QOpenGLTexture(skin);
    m_skinTexture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest);
    m_skinTexture->setMagnificationFilter(QOpenGLTexture::Nearest);

    if(!cape.isNull())
    {
        m_capeTexture = new QOpenGLTexture(cape);
        m_capeTexture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest);
        m_capeTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
    }
    m_skinShader->release();
}

void RenderContext::regenerateGeometry(int skinVersion, Model skinModel)
{
    m_vertexBuffer.clear();
    m_elementBuffer.clear();
    m_translucentTriangles.clear();
    m_elementStartIndex = 0;

    RenderSkin(*this, skinVersion, skinModel);

    for(size_t i = 0; i + 2 < m_elementBuffer.size(); i += 3)
    {
        if(m_vertexBuffer[m_elementBuffer[i]].m_transparency != 0.0f)
        {
            m_translucentTriangles.insert(m_translucentTriangles.end(), m_elementBuffer.begin() + i, m_elementBuffer.begin() + i + 3);
        }
    }

    m_vbo->bind();
    m_vbo->allocate(m_vertexBuffer.data(), m_vertexBuffer.size()*sizeof(Vertex));
    m_vbo->release();

    // room for the sorted translucent triangles after the regular elements
    m_ebo->bind();
    m_ebo->setUsagePattern(QOpenGLBuffer::DynamicDraw);
    m_ebo->allocate((m_elementBuffer.size() + m_translucentTriangles.size())*sizeof(GLuint));
    m_ebo->write(0, m_elementBuffer.data(), m_elementBuffer.size()*sizeof(GLuint));
    m_ebo->release();
}

void RenderContext::sortTranslucentTriangles(const QMatrix4x4& viewMatrix)
{
    struct SortEntry {
        float distance;
        size_t offset;
    };
    std::vector<SortEntry> entries;
    entries.reserve(m_translucentTriangles.size() / 3);
    for(size_t i = 0; i + 2 < m_translucentTriangles.size(); i += 3)
    {
        float distance = 0.0f;
        for(size_t j = 0; j < 3; j++)
        {
            const auto& position = m_vertexBuffer[m_translucentTriangles[i + j]].m_position;
            // w in clip space is the distance along the view direction
            distance += (viewMatrix * QVector4D(position[0], position[1], position[2], 1.0f)).w();
        }
        entries.push_back({distance, i});
    }
    // farthest first
    std::stable_sort(entries.begin(), entries.end(), [](const SortEntry& left, const SortEntry& right) {
        return left.distance > right.distance;
    });

    std::vector<GLuint> sorted;
    sorted.reserve(m_translucentTriangles.size());
    for(const auto& entry: entries)
    {
        sorted.insert(sorted.end(), m_translucentTriangles.begin() + entry.offset, m_translucentTriangles.begin() + entry.offset + 3);
    }
    // the element buffer is part of the bound VAO
    m_ebo->write(m_elementBuffer.size()*sizeof(GLuint), sorted.data(), sorted.size()*sizeof(GLuint));
}

void Skins::RenderContext::render(QMatrix4x4 viewMatrix, QVector3D baseColor, QVector3D alternateColor, float fDevicePixelSize)
{
    GL.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Checkerboard background
    GL.glDisable(GL_DEPTH_TEST);
    GL.glDepthMask(GL_FALSE);
    m_bgShader->bind();
    m_bgShader->setUniformValue(m_bgUniforms[0].id, baseColor);
    m_bgShader->setUniformValue(m_bgUniforms[1].id, alternateColor);
    m_bgShader->setUniformValue(m_bgUniforms[2].id, fDevicePixelSize);
    m_bgvao->bind();
    GL.glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    m_bgvao->release();
    m_bgShader->release();

    /** TODO:
     * - render pure black version of the model (all color except alpha converted to black) to texture
     * - render this texture over the background with a blur effect
     * - render the textured model over that
     */

    // Textured model
    GL.glDepthMask(GL_TRUE);
    GL.glEnable(GL_DEPTH_TEST);
    m_skinShader->bind();
    m_skinShader->setUniformValue(m_uniforms[0].id, viewMatrix);
    m_skinShader->setUniformValue(m_uniforms[1].id, 0);
    m_skinShader->setUniformValue(m_uniforms[2].id, 1);
    m_skinTexture->bind(0);
    if(m_capeTexture)
    {
        m_capeTexture->bind(1);
    }
    m_vao->bind();

    // Opaque texels, with depth writes
    m_skinShader->setUniformValue(m_uniforms[3].id, 0);
    GL.glDrawElements(GL_TRIANGLES, m_elementBuffer.size(), GL_UNSIGNED_INT, nullptr);

    // Partially transparent texels, back to front, blended over everything behind them
    if(!m_translucentTriangles.empty())
    {
        sortTranslucentTriangles(viewMatrix);
        GL.glDepthMask(GL_FALSE);
        m_skinShader->setUniformValue(m_uniforms[3].id, 1);
        GL.glDrawElements(GL_TRIANGLES, m_translucentTriangles.size(), GL_UNSIGNED_INT, reinterpret_cast<const void*>(m_elementBuffer.size()*sizeof(GLuint)));
        GL.glDepthMask(GL_TRUE);
    }

    m_vao->release();
    m_skinShader->release();
    GL.glFinish();
}

}
