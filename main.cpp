#include <QtCore>
#include <QCoreApplication>
#include <QtXml>
#include <QDebug>

#include <iostream>

void retrieveElements(QDomElement root, QString tag, QString att)
{
    QDomNodeList nodes = root.elementsByTagName(tag);
    qDebug() <<"number of nodes "<<nodes.count();
    for (int i = 0; i < nodes.count(); ++i)
    {
        QDomNode elm = nodes.at(i);
        if(elm.isElement())
        {
            QDomElement e = elm.toElement();
            std::cout <<e.attribute(att).toStdString()<<std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);
    QDomDocument doc;
    
    QFile file(argv[1]);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() <<"Failed to open/access file containing SMS";
        return -1;
    }
    else
    {
        if(!doc.setContent(&file))
        {
            qDebug() << "failed to read the file containing the SMS";
            return -2;
        }
        file.close();
    }
    QDomElement root = doc.firstChildElement();
    retrieveElements(root, "sms", "contact_name");//umlauts. In group chat only the first recipient is given
    retrieveElements(root, "sms", "address");//can contain 2+ recipients if in group chat
    retrieveElements(root, "sms", "body");//umlauts
    retrieveElements(root, "sms", "date");
    retrieveElements(root, "sms", "readable_date");//dd.mm.yyyy hh:mm:ss
    retrieveElements(root, "sms", "type");//1 seems to be received, 2 seems to be sent. -1 group chat
    qDebug()<<"Done!";
return a.exec();
}
