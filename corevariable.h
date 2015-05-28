#ifndef COREVARIABLE_H
#define COREVARIABLE_H

#include <QStringList>
#include <string>

#include "libretro.h"

class CoreVariable
{

    QString qmlKey;
    QString qmlDescription;
    QString qmlValue;
    QStringList qmlChoices;

public:
    CoreVariable() {

    }

    explicit CoreVariable( const retro_variable *var );

    QString key() const
    {
        return qmlKey;
    }

    QString value() const
    {
        return qmlValue;
    }

    QString description() const
    {
        return qmlDescription;
    }

    void setValue( const QString value )
    {
        qmlValue = value;
    }

    bool isValid() const;

    QStringList choices() const
    {
        return qmlChoices;
    }


};

#endif // COREVARIABLE_H
