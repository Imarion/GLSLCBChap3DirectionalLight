#include "dirlight.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <cstring>

MyWindow::~MyWindow()
{
    if (mProgramDirLight != 0) delete mProgramDirLight;
}

MyWindow::MyWindow()
    : mProgramDirLight(0), currentTimeMs(0), currentTimeS(0)
{
    worldLight = QVector4D(1.0f, 0.0f, 0.0f, 0.0f); // by default directional light

    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    mFuncs->glGenVertexArrays(1, &mVAO);
    mFuncs->glBindVertexArray(mVAO);

    CreateVertexBuffer();
    initShaders();
    initMatrices();

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{
    mTorus = new Torus(0.7f, 0.3f, 50, 50);

    // Create and populate the buffer objects
    unsigned int handle[3];
    glGenBuffers(3, handle);        

    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTorus->getnVerts()) * sizeof(float), mTorus->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTorus->getnVerts()) * sizeof(float), mTorus->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mTorus->getnFaces() * sizeof(unsigned int), mTorus->getel(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, handle[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, handle[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[2]);

    mFuncs->glBindVertexArray(0);
}

void MyWindow::initMatrices()
{
    /*
    ModelMatrixDirLight.translate(1.6f, 0.5f, 0.0f);
    ModelMatrixDirLight.rotate( -90.0f, QVector3D(1.0f,0.0f,0.0f));
    ModelMatrixDirLight.scale(0.50f);
    */
    ModelMatrixDirLight.rotate( 90.0f, QVector3D(0.0f, 1.0f, 0.0f));

    ViewMatrix.lookAt(QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.0f,0.0f,0.0f), QVector3D(0.0f,1.0f,0.0f));
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(70.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    static float EvolvingVal = 0;
    EvolvingVal += 0.1f;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mFuncs->glBindVertexArray(mVAO);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgramDirLight->bind();
    {        
        mProgramDirLight->setUniformValue("Light.Position", ViewMatrix * worldLight );
        mProgramDirLight->setUniformValue("Light.Intensity", 0.8f, 0.8f, 0.8f );

        mProgramDirLight->setUniformValue("Material.Kd", 0.8f, 0.8f, 0.8f);
        mProgramDirLight->setUniformValue("Material.Ks", 0.9f, 0.9f, 0.9f);
        mProgramDirLight->setUniformValue("Material.Ka", 0.1f, 0.1f, 0.1f);
        mProgramDirLight->setUniformValue("Material.Shininess", 180.0f);

        QMatrix4x4 mv2 = ViewMatrix * ModelMatrixDirLight;
        mProgramDirLight->setUniformValue("ModelViewMatrix", mv2);
        mProgramDirLight->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgramDirLight->setUniformValue("MVP", ProjectionMatrix * mv2);

        glDrawElements(GL_TRIANGLES, 6 * mTorus->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgramDirLight->release();

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    // Directional light
    shaderFile.setFileName(":/vshader_dirlight.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex directional light compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_dirlight.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   directional light compile: " << fShader.compileSourceCode(shaderSource);

    mProgramDirLight = new (QOpenGLShaderProgram);
    mProgramDirLight->addShader(&vShader);
    mProgramDirLight->addShader(&fShader);
    qDebug() << "shader link directional light: " << mProgramDirLight->link();

}

void MyWindow::PrepareTexture(GLenum TextureTarget, const QString& FileName, GLuint& TexObject, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture";
    if (flip==true) TexImg=TexImg.mirrored();

    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameterf(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            worldLight.setW(1.0f-worldLight.w());            
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
