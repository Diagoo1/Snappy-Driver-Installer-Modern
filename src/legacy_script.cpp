#include "legacy_script.h"
#include <QTextCodec>
#include <QRegularExpression>
#include <QJsonArray>
#include <QProcessEnvironment>
#include <QSet>
#include <QFile>
#include <stdexcept>
#include <cstring>
namespace Legacy {
static void fail(const QString &value){throw std::runtime_error(value.toUtf8().constData());}
QStringList tokenize(const QString &text){
    QStringList result;QString token;bool quote=false,started=false;
    for(int i=0;i<text.size();++i){const QChar c=text[i];if(c==QLatin1Char('"')){quote=!quote;started=true;continue;}if(c.isSpace() && !quote){if(started){result<<token;token.clear();started=false;}}else{token+=c;started=true;}}
    if(quote)fail(QStringLiteral("Unclosed script quotation."));if(started)result<<token;return result;
}
QStringList expandConfiguration(const QStringList &arguments,int depth){
    if(depth>4)fail(QStringLiteral("Legacy configuration nesting is too deep."));QStringList result;
    for(const QString &argument:arguments){if(argument.startsWith(QStringLiteral("-cfg:"),Qt::CaseInsensitive) || argument.startsWith(QStringLiteral("/cfg:"),Qt::CaseInsensitive)){
        QFile file(argument.mid(5));if(!file.open(QIODevice::ReadOnly) || file.size()>1024*1024)fail(QStringLiteral("Cannot read the bounded legacy configuration."));QString text=QTextCodec::codecForUtfText(file.peek(4),QTextCodec::codecForName("UTF-8"))->toUnicode(file.readAll());text.remove(QChar(0xfeff));QStringList lines;for(const QString &line:text.split(QLatin1Char('\n'))){const QString clean=line.trimmed();if(!clean.startsWith(QLatin1Char('#')) && !clean.startsWith(QLatin1Char(';')))lines<<clean;}
        result<<expandConfiguration(tokenize(lines.join(QStringLiteral(" "))),depth+1);
    }else result<<argument;}
    if(result.size()>4096)fail(QStringLiteral("Too many legacy configuration arguments."));return result;
}
static void validate(const Command &command){
    const QString n=command.name;const QStringList a=command.arguments;const int count=a.size();int minimum=0,maximum=0;
    if(n==QStringLiteral("init")){maximum=1;if(count && a[0].toLower()!=QStringLiteral("reindex"))fail(QStringLiteral("init accepts only reindex."));}
    else if(n==QStringLiteral("select") || n==QStringLiteral("cmd")){minimum=1;maximum=128;}
    else if(n==QStringLiteral("get")){minimum=1;maximum=2;const QString kind=a.value(0).toLower();if(!QStringList({QStringLiteral("app"),QStringLiteral("indexes"),QStringLiteral("driverpacks"),QStringLiteral("everything")}).contains(kind))fail(QStringLiteral("Unknown get target."));if(kind==QStringLiteral("driverpacks") && !QStringList({QStringLiteral("all"),QStringLiteral("missing"),QStringLiteral("updates"),QStringLiteral("selected")}).contains(a.value(1).toLower()))fail(QStringLiteral("get driverpacks needs all, missing, updates or selected."));}
    else if(n==QStringLiteral("snapshot")){maximum=1;}
    else if(n==QStringLiteral("loadsnapshot") || n==QStringLiteral("writedevicelist") || n==QStringLiteral("goto") || n==QStringLiteral("logdir") || n==QStringLiteral("drpdir") || n==QStringLiteral("indexdir") || n==QStringLiteral("extractdir")){minimum=1;maximum=128;}
    else if(n==QStringLiteral("restorepoint") || n==QStringLiteral("echo") || n==QStringLiteral("runlatest")){maximum=128;}
    else if(n==QStringLiteral("activetorrent") || n==QStringLiteral("torrentport") || n==QStringLiteral("verbose")){minimum=1;maximum=1;bool ok=false;const int value=a.value(0).toInt(&ok);if(!ok || value<0 || (n==QStringLiteral("activetorrent") && value>1) || (n==QStringLiteral("torrentport") && value>65535))fail(QStringLiteral("Invalid script numeric option."));}
    else if(n==QStringLiteral("enableinstall") || n==QStringLiteral("keeptempfiles") || n==QStringLiteral("debug") || n==QStringLiteral("logging")){maximum=1;if(count && a[0].toLower()!=QStringLiteral("on") && a[0].toLower()!=QStringLiteral("off"))fail(QStringLiteral("Script switch accepts on or off."));}
    else if(n==QStringLiteral("onerror")){minimum=1;maximum=2;if(a.value(0).toLower()!=QStringLiteral("end") && !(a.value(0).toLower()==QStringLiteral("goto") && count==2))fail(QStringLiteral("onerror needs end or goto label."));}
    else if(n==QStringLiteral("reboot")){maximum=1;if(count && a[0].toLower()!=QStringLiteral("ifneeded"))fail(QStringLiteral("reboot accepts only ifneeded."));}
    else if(n!=QStringLiteral("checkupdates") && n!=QStringLiteral("install") && n!=QStringLiteral("unloadsnapshot") && n!=QStringLiteral("pause") && n!=QStringLiteral("end"))fail(QStringLiteral("Unknown legacy command: ")+n);
    if(n==QStringLiteral("goto"))maximum=1;
    if(count<minimum || count>maximum)fail(QStringLiteral("Wrong argument count for ")+n+QStringLiteral(" at line ")+QString::number(command.line));
}
Program parse(const QByteArray &bytes,const QStringList &parameters){
    if(bytes.size()>1024*1024)fail(QStringLiteral("Script exceeds one MiB."));QString text;
    if(bytes.startsWith("\xff\xfe") || bytes.startsWith("\xfe\xff"))text=QTextCodec::codecForName("UTF-16")->toUnicode(bytes);
    else {QTextCodec::ConverterState state;text=QTextCodec::codecForName("UTF-8")->toUnicode(bytes.constData(),bytes.size(),&state);if(state.invalidChars)text=QString::fromLocal8Bit(bytes);}
    text.remove(QChar(0xfeff));const QStringList lines=text.split(QLatin1Char('\n'));if(lines.size()>20000)fail(QStringLiteral("Script has too many lines."));
    Program result;const QProcessEnvironment environment=QProcessEnvironment::systemEnvironment();
    for(int line=0;line<lines.size();++line){QString value=lines[line].trimmed();if(value.isEmpty() || value.startsWith(QLatin1Char('#')) || value.startsWith(QLatin1Char(';')))continue;
        value.replace(QRegularExpression(QStringLiteral("%([0-9])")),QStringLiteral("@@SDIO_PARAMETER_\\1@@"));for(int i=0;i<10;++i)value.replace(QStringLiteral("@@SDIO_PARAMETER_%1@@").arg(i),parameters.value(i));
        const QRegularExpression pattern(QStringLiteral("%([A-Za-z_][A-Za-z0-9_]*)%"));auto variables=pattern.globalMatch(value);QVector<QPair<QString,QString> > replacements;while(variables.hasNext()){const auto variable=variables.next();const QString key=variable.captured(1);for(const QString &name:environment.keys())if(name.compare(key,Qt::CaseInsensitive)==0){replacements.append(qMakePair(variable.captured(),environment.value(name)));break;}}for(const auto &replacement:replacements)value.replace(replacement.first,replacement.second);
        if(value.size()>65536)fail(QStringLiteral("Expanded script line is too long."));const QStringList words=tokenize(value);if(words.isEmpty())continue;
        Command command;command.name=words[0].toLower();command.arguments=words.mid(1);command.raw=value.mid(value.indexOf(words[0],0,Qt::CaseInsensitive)+words[0].size()).trimmed();command.line=line+1;
        if(command.name.startsWith(QLatin1Char(':'))){const QString label=command.name.mid(1);if(words.size()!=1 || label.isEmpty() || result.labels.contains(label))fail(QStringLiteral("Invalid or duplicate script label."));result.labels.insert(label,result.commands.size());continue;}
        validate(command);result.commands.append(command);
    }
    for(const Command &command:result.commands)if(command.name==QStringLiteral("goto") || (command.name==QStringLiteral("onerror") && command.arguments.value(0).toLower()==QStringLiteral("goto"))){const QString label=command.arguments.last().toLower().remove(QRegularExpression(QStringLiteral("^:")));if(!result.labels.contains(label))fail(QStringLiteral("Script label was not found: ")+label);}
    return result;
}
QJsonObject describe(const Program &program){QJsonObject result;QJsonArray commands;for(const Command &command:program.commands){QJsonObject item;item.insert(QStringLiteral("line"),command.line);item.insert(QStringLiteral("command"),command.name);item.insert(QStringLiteral("arguments"),QJsonArray::fromStringList(command.arguments));commands.append(item);}result.insert(QStringLiteral("commands"),commands);result.insert(QStringLiteral("validated"),true);result.insert(QStringLiteral("executed"),false);return result;}
int execute(const Program &program,const std::function<int(const Command&)> &effect,int maximum){
    int last=0,position=0,steps=0;while(position<program.commands.size()){
        if(++steps>maximum)fail(QStringLiteral("Script execution step limit reached."));const Command &command=program.commands[position++];const QString n=command.name;
        if(n==QStringLiteral("end"))break;
        if(n==QStringLiteral("onerror") && last==0)continue;
        if(n==QStringLiteral("onerror") && command.arguments[0].toLower()==QStringLiteral("end"))break;
        if(n==QStringLiteral("goto") || n==QStringLiteral("onerror")){const QString label=command.arguments.last().toLower().remove(QRegularExpression(QStringLiteral("^:")));position=program.labels.value(label);continue;}
        last=effect(command);
    }return last;
}
bool requested(int argc,char **argv){for(int i=1;i<argc;++i){const QByteArray arg=QByteArray(argv[i]).toLower();if(arg=="--validate-script" || arg=="--legacy-report" || arg.startsWith("-script:") || arg.startsWith("/script:") || (arg.startsWith('-') && !arg.startsWith("--") && arg!="-platform") || (arg.startsWith('/') && arg.size()>1 && arg[1]!='/' && arg.mid(1).indexOf('/')<0))return true;}return false;}
}
