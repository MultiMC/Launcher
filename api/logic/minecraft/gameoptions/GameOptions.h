#pragma once

#include <map>
#include <QString>
#include <QAbstractListModel>

struct RawGameOptions
{
    void clear()
    {
        version = 0;
        mapping.clear();
    }
    std::map<QString, QString> mapping;
    int version = 0;
};

struct GameOptionItem
{
    QString id;
    enum ValueType
    {
        INT,
        FLOAT,
        BOOL,
        FOV_MADNESS,
        FPS_MADNESS
    } value_type;
    enum VisualType
    {

    } visual_type;
    QVariant null_value;
    QVariant default_value;
    QVariant min_value;
    QVariant max_value;
    QString key;
};

struct CookedGameOptions
{
    std::vector<GameOptionItem> items;
};



class GameOptions : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit GameOptions(const QString& path);
    virtual ~GameOptions() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    bool isLoaded() const;
    bool reload();
    bool save();

private:
    RawGameOptions rawOptions;
    CookedGameOptions cookedOptions;
    bool loaded = false;
    QString path;
    int version = 0;
};
