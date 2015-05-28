#include "corevariable.h"
#include "logging.h"

#include <QStringList>
#include <QDebug>

CoreVariable::CoreVariable(const retro_variable *var)
{

    qmlKey = var->key;

    // "Text before first ';' is description. This ';' must be followed by a space,
    // and followed by a list of possible values split up with '|'."
    QString valDesc( var->value );

    QStringList valDescList = valDesc.split(";");

    if ( valDescList.length() > 2 )
        qCWarning(phxCore) << "Core provided variables do not follow the API!";

    qmlDescription = valDescList.at(0);
    QStringList values = valDescList.at(1).simplified().split("|");

    for ( auto &val : values ) {
        qmlChoices.append( val );
    }

}

bool CoreVariable::isValid() const
{
    return !qmlKey.isEmpty();
}
