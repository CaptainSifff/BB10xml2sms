/*
 * XML2SMS.h
 *
 *  Created on: Nov 1, 2016
 *      Author: sifff
 */

#ifndef XML2SMS_H_
#define XML2SMS_H_
#include <QtCore/QObject>

class XML2SMS: public QObject
{
    Q_OBJECT
public:
    explicit XML2SMS (QObject* parent = 0);
    virtual ~XML2SMS();
public Q_SLOTS:
    void onXMLFileSelect(const QString);
};

#endif /* XML2SMS_H_ */
