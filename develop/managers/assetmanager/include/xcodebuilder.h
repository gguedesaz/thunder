#ifndef XCODEBUILDER_H
#define XCODEBUILDER_H

#include <converters/builder.h>

class QProcess;

class XcodeBuilder : public IBuilder {
public:
    XcodeBuilder ();

    bool buildProject ();

    QString builderVersion ();

    QStringList suffixes () const { return {"cpp", "h"}; }

private:
    void generateProject();

private:
    QProcess *m_pProcess;

    QStringList m_Settings;

    ProjectManager *m_pMgr;

    bool m_Progress;
};

#endif // XCODEBUILDER_H
