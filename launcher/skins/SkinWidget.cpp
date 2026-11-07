/* Copyright 2025 Petr Mrázek
 *
 * This source is subject to the Microsoft Permissive License (MS-PL).
 * Please see the COPYING.md file for more information.
 */

#include "SkinWidget.h"
#include "SkinRenderer.h"

#include <cmath>

#include <QtCore/QDebug>
#include <QVector3D>
#include <QVBoxLayout>


SkinWidget::SkinWidget(QWidget* parent): QFrame(parent)
{
    // TODO: if OpenGL init fails in the Window, this should provide a fallback with no 3D rendering
    m_window = new SkinWindow();
    m_window->setBackgroundColor(palette().color(QPalette::Normal, QPalette::Base));
    auto mainWidget = QWidget::createWindowContainer(m_window, this);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(mainWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);
}

bool SkinWidget::event(QEvent* event)
{
    if(event->type() == QEvent::Type::PaletteChange)
    {
        m_window->setBackgroundColor(palette().color(QPalette::Normal, QPalette::Base));
    }
    return QFrame::event(event);
}


void SkinWidget::setAll(Skins::Model model, const QImage& skinImage, const QImage& capeImage)
{
    m_window->setAll(model, skinImage, capeImage);
}

void SkinWidget::setCapeImage(const QImage& capeImage)
{
    m_window->setCapeImage(capeImage);
}

void SkinWidget::setModel(Skins::Model model)
{
    m_window->setModel(model);
}

void SkinWidget::setSkinImage(const QImage& skinImage)
{
    m_window->setSkinImage(skinImage);
}
void SkinWidget::setBackgroundColor(QColor color)
{
    m_window->setBackgroundColor(color);
}


SkinWindow::SkinWindow(): QOpenGLWindow()
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(3);
    format.setMinorVersion(2);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    setFormat(format);

    m_context = new Skins::RenderContext(GL, this);
    m_spinAngle = 0.0f;
    m_liftAngle = 0.0f;
    m_distance = 48.0f;

    setAll(Skins::Model::Classic, QImage(":/skins/textures/placeholder_skin.png"), QImage(":/skins/textures/placeholder_cape.png"));
}

SkinWindow::~SkinWindow() noexcept
{
    m_context->deinit();
}


void SkinWindow::initializeGL()
{
    GL.initializeOpenGLFunctions();
    m_context->init();
}

void SkinWindow::resizeGL(int w, int h)
{
    const qreal retinaScale = devicePixelRatioF();
    GL.glViewport(0, 0, w * retinaScale, h * retinaScale);
    m_height = h;
    m_width = w;

    m_projection.setToIdentity();
    float aspect = float(w) / float(h);
    m_projection.perspective(45.0f, aspect, 0.1f, 100.0f);
    updateMatrix();
}

void SkinWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_lastPos = event->pos();
    }
}

void SkinWindow::mouseMoveEvent(QMouseEvent *event)
{
    auto position = event->pos();
    if (event->buttons() & Qt::LeftButton) {
        int dx = position.x() - m_lastPos.x();
        int dy = position.y() - m_lastPos.y();

        m_spinAngle -= dx * 0.5 * 0.01;
        m_liftAngle += dy * 0.5 * 0.01;
        m_lastPos = event->pos();
        updateMatrix();
        update();
    }
}

void SkinWindow::wheelEvent(QWheelEvent *event)
{
    float numDegrees = float(event->angleDelta().y()) / 8.0;
    float numSteps = numDegrees / 15.0;
    m_distance -= numSteps;
    updateMatrix();
    update();
}


// OK LAB color space conversions for decent determination of lighter/darker alternate color
// NOTE: code adapted from https://bottosson.github.io/posts/oklab/
namespace {
QVector3D linear_srgb_to_oklab(QVector3D c)
{
    float l = 0.4122214708f * c.x() + 0.5363325363f * c.y() + 0.0514459929f * c.z();
    float m = 0.2119034982f * c.x() + 0.6806995451f * c.y() + 0.1073969566f * c.z();
    float s = 0.0883024619f * c.x() + 0.2817188376f * c.y() + 0.6299787005f * c.z();

    float l_ = cbrtf(l);
    float m_ = cbrtf(m);
    float s_ = cbrtf(s);

    return {
        0.2104542553f*l_ + 0.7936177850f*m_ - 0.0040720468f*s_,
        1.9779984951f*l_ - 2.4285922050f*m_ + 0.4505937099f*s_,
        0.0259040371f*l_ + 0.7827717662f*m_ - 0.8086757660f*s_,
    };
}

QVector3D oklab_to_linear_srgb(QVector3D c)
{
    float l_ = c.x() + 0.3963377774f * c.y() + 0.2158037573f * c.z();
    float m_ = c.x() - 0.1055613458f * c.y() - 0.0638541728f * c.z();
    float s_ = c.x() - 0.0894841775f * c.y() - 1.2914855480f * c.z();

    float l = l_*l_*l_;
    float m = m_*m_*m_;
    float s = s_*s_*s_;

    return {
        +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s,
        -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s,
        -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s,
    };
}

QVector3D getContrastColor(QVector3D color) {
    constexpr float contrast = 0.035;
    auto lab = linear_srgb_to_oklab(color);
    if(lab.x() < contrast)
    {
        lab.setX(lab.x() + contrast);
    }
    else
    {
        lab.setX(lab.x() - contrast);
    }
    return oklab_to_linear_srgb(lab);
}
}

void SkinWindow::paintGL()
{
    if(m_skinDirty)
    {
        auto version = m_skinImage.height() >= 64 ? 1 : 0;
        m_context->setTextures(m_skinImage, m_capeImage);
        m_context->regenerateGeometry(version, m_model);
        m_skinDirty = false;
    }

    QVector3D alternateColor = getContrastColor(m_backgroundColor);
    m_context->render(m_worldToView, m_backgroundColor, alternateColor, devicePixelRatioF());
}

void SkinWindow::updateMatrix() {
    QMatrix4x4 camera;
    QVector3D cameraPosition(
        m_distance * sin(m_spinAngle) * cos(m_liftAngle),
        m_distance * sin(m_liftAngle),
        m_distance * cos(m_spinAngle) * cos(m_liftAngle)
    );
    QVector3D targetPosition(0.0f, 0.0f, 0.0f);
    QVector3D upVector(0.0f, 1.0f, 0.0f);
    camera.lookAt(cameraPosition, targetPosition, upVector);
    m_worldToView = m_projection * camera;
}

void SkinWindow::setAll(Skins::Model model, const QImage& skinImage, const QImage& capeImage)
{
    m_model = model;
    m_skinImage = skinImage;
    m_capeImage = capeImage;
    m_skinDirty = true;
    update();
}

void SkinWindow::setCapeImage(const QImage& capeImage)
{
    m_capeImage = capeImage;
    m_skinDirty = true;
    update();
}

void SkinWindow::setSkinImage(const QImage& skinImage)
{
    m_skinImage = skinImage;
    m_skinDirty = true;
    update();
}

void SkinWindow::setModel(Skins::Model model)
{
    m_model = model;
    m_skinDirty = true;
    update();
}

void SkinWindow::setBackgroundColor(QColor color)
{
    m_backgroundColor = QVector3D(color.redF(), color.greenF(), color.blueF());
    update();
}
