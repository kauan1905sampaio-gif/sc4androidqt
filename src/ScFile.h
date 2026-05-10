#pragma once
#include <QString>
#include <QVector>
#include <QByteArray>

enum class Compression { None, LZMA, ZSTD, LZHAM };
enum class PixelFormat { RGBA8888, RGBA4444, RGB565, LA88, L8 };

struct Vertex   { float x, y; };
struct UvCoord  { float u, v; };

struct ShapeChunk {
    int id = 0, textureIndex = 0;
    QVector<Vertex>  vertices;
    QVector<UvCoord> uvCoords;
};

struct Shape {
    int id = 0;
    QVector<ShapeChunk> chunks;
};

struct FrameElement {
    int shapeId = 0, matrixIndex = 0, colorSpaceIndex = 0;
};

struct MovieclipFrame {
    QString name;
    QVector<FrameElement> elements;
};

struct Movieclip {
    int id = 0, fps = 25;
    QVector<MovieclipFrame> frames;
};

struct Export {
    int id = 0, movieclipId = 0;
    QString name;
};

struct Matrix {
    int id = 0;
    float a=1,b=0,c=0,d=1,tx=0,ty=0;
};

struct ColorSpace {
    int id = 0;
    int rMul=255,gMul=255,bMul=255,aMul=255;
    int rAdd=0,gAdd=0,bAdd=0,aAdd=0;
};

struct TextField {
    int id=0, fontSize=12;
    QString text, fontName;
    bool bold=false, italic=false;
};

struct ScTexture {
    int index=0, width=0, height=0;
    PixelFormat pixelFormat = PixelFormat::RGBA8888;
    QByteArray data;
};

struct ScFile {
    QString path;
    int version = 3;
    Compression compression = Compression::None;
    QVector<Export>     exports;
    QVector<Movieclip>  movieclips;
    QVector<Shape>      shapes;
    QVector<ScTexture>  textures;
    QVector<Matrix>     matrices;
    QVector<ColorSpace> colorSpaces;
    QVector<TextField>  textFields;
};
