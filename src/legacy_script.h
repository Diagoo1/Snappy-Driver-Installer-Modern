#ifndef SDIO_LEGACY_SCRIPT_H
#define SDIO_LEGACY_SCRIPT_H
#include <QStringList>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <functional>
namespace Legacy {
struct Command {QString name,raw;QStringList arguments;int line=0;};
struct Program {QVector<Command> commands;QMap<QString,int> labels;};
QStringList tokenize(const QString&);
QStringList expandConfiguration(const QStringList&,int depth=0);
Program parse(const QByteArray&,const QStringList &parameters=QStringList());
QJsonObject describe(const Program&);
// Runtime effects are injected; tests validate control flow without touching Windows.
int execute(const Program&,const std::function<int(const Command&)> &,int maximumSteps=100000);
bool requested(int argc,char **argv);
int run(int argc,char **argv);
}
#endif
