/*
 * DataForm.h - Data Forms (XEP-0004)
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef XMPP_DATA_FORM_H_
#define XMPP_DATA_FORM_H_

#include <QList>
#include <QSharedDataPointer>

class QDomElement;
class QStringList;

#define NS_DATA_FORMS "jabber:x:data"

namespace XMPP
{


class DataForm
{
    public:
        enum Type { Invalid, Form, Submit, Cancel, Result };

        class Field;
        class Item;

        DataForm();
        DataForm(const DataForm& other);
        virtual ~DataForm();
        DataForm& operator=(const DataForm& other);

        bool isEmpty() const;
        bool isValid() const;

        static DataForm fromDomElement(const QDomElement& form);
        void toDomElement(QDomElement& element) const;

        void addField(const Field& field);
        void addItem(const Item& item);

        DataForm& operator<<(const Field& field);
        DataForm& operator<<(const Item& item);

        QList<Field> fields() const;
        QList<Item> items() const;

        QList<Field> reportedFields() const;
        void addReportedField(const Field& field);
        void setReportedFields(const QList<Field>& reported);
        void setReportedFields(const Field& field);

        Type type() const;
        QString title() const;
        QString instructions() const;

        void setType(Type type);
        void setTitle(const QString& title);
        void setInstructions(const QString& instructions);

        Field fieldByName(const QString& name);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};

class DataForm::Field
{
    public:
        enum FieldType { Invalid, Boolean, Fixed, Hidden, JidMulti, JidSingle, ListMulti, ListSingle, TextMulti, TextPrivate, TextSingle };

        Field();
        Field(const Field& other);
        Field(const QString& name, FieldType type = TextSingle);
        Field(const QString& name, const QString& label, FieldType type = TextSingle);
        virtual ~Field();

        static Field fromNameValue(const QString& name, const QString& value, FieldType type = TextSingle);
        static Field fromNameLabelValue(const QString& name, const QString& label, const QString& value, FieldType type = TextSingle);

        static Field fromDomElement(const QDomElement& field);
        void toDomElement(QDomElement& element) const;

        void addOption(const QString& optionLabel, const QString& value);
        void addValue(const QString& value);

        QStringList values() const;

        FieldType type() const;
        void setType(FieldType type);

        bool isRequred() const;
        void setRequired(bool required = true);

        QString label() const;
        void setLabel(const QString& label);

        QString name() const;
        void setName(const QString& name);

        QString desc() const;
        void setDesc(const QString& desc);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};

class DataForm::Item
{
    public:
        Item();
        Item(const Item& other);
        virtual ~Item();

        static Item fromDomElement(const QDomElement& item);
        void toDomElement(QDomElement& element) const;

        void addField(const Field& field);
        Item& operator<<(const Field& field);
        QList<Field> fields() const;
        void setFields(const QList<Field>& flist);
    private:
        class Private;
        QSharedDataPointer<Private> d;
};


}

// vim:ts=4:sw=4:nowrap:et
#endif /* XMPP_DATA_FORM_H_ */
