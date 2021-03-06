/*
 * XML2SMS.cpp
 *
 *  Created on: Nov 1, 2016
 *      Author: sifff
 */

#include <src/XML2SMS.h>
#include <QtXml/QDomDocument>
#include <QtCore/QFile>
#include <QDebug>
#include <QDateTime>
#include <QStringList>
#include <QMap>
#include <bb/pim/message/MessageService>
#include <bb/pim/message/MessageFolder>
#include <bb/pim/message/MessageStatus>
#include <bb/pim/account/AccountService>
#include <bb/pim/account/Account>
#include <bb/pim/account/Service>
#include <bb/pim/message/ConversationBuilder>
#include <bb/pim/message/MessageBuilder>
#include <bb/pim/message/MessageFilter>
#include <bb/pim/message/MessageContact>
#include <bb/pim/message/Conversation>

#include <bb/pim/contacts/ContactService>
#include <bb/pim/contacts/Contact>
#include <bb/pim/contacts/ContactSearchFilters>
#include <bb/pim/contacts/ContactConsts>
#include <bb/pim/account/Result>

#include <bb/system/SystemToast>

using namespace bb::pim;

XML2SMS::XML2SMS(QObject* parent) : QObject(parent){}

XML2SMS::~XML2SMS()
{
    // TODO Auto-generated destructor stub
}

void XML2SMS::onXMLFileSelect(const QString qs)
{
    bb::system::SystemToast toast;
    QDomDocument doc;
qDebug()<<qs;
        QFile file(qs);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qDebug() <<"Failed to open/access file containing SMS";
            return;
        }
        else
        {
            if(!doc.setContent(&file))
            {
                qDebug() << "failed to read the file containing the SMS";
//                return -2;
            }
            else
                qDebug()<<"success loading file!";
            file.close();
        }
        QMap<QString, message::ConversationKey> conversationidmap;
        message::MessageService messageService;
        account::AccountService accountService;
        //Get the SMS/MMS account
        QList<account::Account> accountList = accountService.accounts(account::Service::Messages,"sms-mms");
        account::AccountKey smsaccount =  accountList.first().id();
        QList<account::AccountKey> accountidlist;
        accountidlist.append(smsaccount);
        QDomElement root = doc.firstChildElement();
        QDomNodeList nodes = root.elementsByTagName("sms");
        if (nodes.count() == 0)
        {
            toast.setBody("No SMS found in file.");
            toast.setPosition(bb::system::SystemUiPosition::MiddleCenter);
            toast.exec();
            return;
        }
        qDebug() <<"number of nodes "<<nodes.count();
        int importedsms = 0;
        for (int i = 0; i < /*nodes.count()*/ 15; ++i)
        {
            QDomNode elm = nodes.at(i);
            if(elm.isElement())
            {
            message::ConversationBuilder* conversationBuilder = message::ConversationBuilder::create();
            // Make a Message object for the sms-mms account
            message::MessageBuilder* msg_builder = message::MessageBuilder::create(smsaccount);
            conversationBuilder->accountId(smsaccount);
            QList<message::MessageContact> participants;
            QDomElement e = elm.toElement();
            //retrieve a list contaning the phonenumber(s)
            QStringList phonenumbers = (e.attribute("address")).split(";");
            for(typename QStringList::const_iterator pn = phonenumbers.begin(); pn != phonenumbers.end(); ++pn)
            {
                contacts::ContactService contactservice;
                contacts::ContactSearchFilters filter;
                filter.setLimit(1);
                QList< contacts::SearchField::Type > fields;
                fields << contacts::SearchField::Phone;
                filter.setSearchFields(fields);
                filter.setSearchValue(*pn);
                QList<contacts::Contact> contact = contactservice.searchContactsByPhoneNumber(filter);
                if (contact.size() >= 1)
qDebug()<<contact.size()<<" "<<contact.first().displayName();
                if(e.attribute("type").toInt() == 1)
                {
                    qDebug()<<"Received from "<<*pn;
                    if (contact.size() >= 1)
                        qDebug()<<"Whois "<<contact.first().displayName();
                    else
                        qDebug()<<"not known";
                    message::MessageContact sender = message::MessageContact(contact.size() != 1?-1:contact.first().id(),
                                            message::MessageContact::From
                                                                             , *pn, *pn);
                    participants.append(sender);
                    msg_builder->sender(sender);
                }
                else
                {
                    qDebug()<<"Sent to "<<*pn;
                    if (contact.size() >= 1)
                        qDebug()<<"Whois "<<contact.first().displayName();
                    else
                        qDebug()<<"not known";
                    message::MessageContact recipient = message::MessageContact(contact.size() != 1?-1:contact.first().id(),
                                            message::MessageContact::To
                                                                             , *pn, *pn);
                    participants.append(recipient);
                    // Specify the recipient of the message (i.e. the destination address/MSISDN)
                    msg_builder->addRecipient(recipient);
                }
                message::MessageContact recipient = message::MessageContact(contact.size() != 1?-1:contact.first().id(),
                        e.attribute("type").toInt()==1?message::MessageContact::From : message::MessageContact::To
                                                         , *pn, *pn);
            }
            conversationBuilder->participants(participants);
            message::Conversation conversation = *conversationBuilder;
            //try to find a chat containing the single participant. Else we create a new one.
            message::ConversationKey conversationid;
            if(phonenumbers.size() == 1)
            {
                QMap<QString, message::ConversationKey>::iterator it = conversationidmap.find(phonenumbers.first());
                if(it != conversationidmap.end())
                    conversationid = it.value();
                else
                {
                    qDebug()<<"Number "<<phonenumbers.first()<<"not known";
                    contacts::ContactService contactservice;
                    contacts::ContactSearchFilters filter;
                    filter.setLimit(1);
                    QList< contacts::SearchField::Type > fields;
                    fields << contacts::SearchField::Phone;
                    filter.setSearchFields(fields);
                    filter.setSearchValue(phonenumbers.first());
                    QList<contacts::Contact> contact = contactservice.searchContactsByPhoneNumber(filter);
                    qDebug()<<"matching contacts: "<<contact.count();
                    if(contact.count() == 1)
                    {
                        message::MessageFilter mf;
                        mf.insert(message::MessageFilter::ContactId, QVariant(contact.first().id()));
                        QList<message::ConversationKey> filteredKeys = messageService.conversationKeys(smsaccount, mf);
                        qDebug()<<"matching conversations "<<filteredKeys.count();
                        if(filteredKeys.count() < 1)
                        {
                            conversationid = messageService.save(smsaccount, conversation);
                            conversationidmap.insert(phonenumbers.first(), conversationid);
                        }
                        else
                        {
                            conversationid = filteredKeys.first();
                            conversationidmap.insert(phonenumbers.first(), conversationid);
                        }
                    }
                    else// multiple accounts match that number
                    {
                        conversationid = messageService.save(smsaccount, conversation);
                        conversationidmap.insert(phonenumbers.first(), conversationid);
                    }
//                    conversationid = messageService.save(smsaccount, conversation);
//                    conversationidmap.insert(phonenumbers.first(), conversationid);
                }

            }
            else
            {
                conversationid = messageService.save(smsaccount, conversation);
            }

            // Set the message’s conversation ID
            msg_builder->conversationId(conversationid);

            // Set the message body (message text) as an Attachment to the Message object
            msg_builder->addAttachment(message::Attachment("text/plain", "", e.attribute("body")));

            //add the proper date of sending
            QDateTime date;
            date.setMSecsSinceEpoch(e.attribute("date").toLongLong());
            msg_builder->deviceTimestamp(date);

//prepare to finally store everything
            message::Message message;
            message = *msg_builder;
            message::MessageKey messagekey = messageService.save(smsaccount, message);
            ++importedsms;
            //add the state wether we sent it or we have received it
            switch(e.attribute("type").toInt())
            {
                case -1:
                    messageService.setStatus(smsaccount, messagekey, message::MessageStatus::Broadcast);
                    break;
                    case 1:
                        messageService.setStatus(smsaccount, messagekey, message::MessageStatus::Read);
                        break;
                    case 2:
                        messageService.setStatus(smsaccount, messagekey, message::MessageStatus::Sent);
                        break;
                    default:
                        break;
            }
//            messageService.syncAccounts(accountidlist);
//            account::Result res = accountService.syncAccounts(account::Service::Messages);
            }
        }
        QString msg("Done importing SMS. Count: ");
        msg += QString::number(importedsms) + " of " + QString::number(nodes.count());
        toast.setBody(msg);
        toast.setPosition(bb::system::SystemUiPosition::MiddleCenter);
        toast.exec();
}
