/*
 * dataform.cpp - Data Forms (XEP-0004)
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

#include "dataform.h"

#include <QDomElement>
#include <QHash>
#include <QString>
#include <QStringList>

namespace XMPP
{

/* ***************************************************************************
 * DataForm::Field
 *************************************************************************** */

class DataForm::Private : public QSharedData
{
    public:
        Private();
        Private(const Private& other);
        ~Private();

        static QString typeToString(Type type);
        static Type stringToType(const QString& type);

        QString title, instructions;
        Type type;

        QList<Field> fields;
        QList<Field> reported;
        QList<Item> items;
};

DataForm::Private::Private()
    : QSharedData()
{
    type = Form;
}

DataForm::Private::Private(const Private& other)
    : QSharedData(other)
{
    title = other.title;
    instructions = other.instructions;

    type = other.type;

    fields = other.fields;
    items = other.items;
    reported = other.reported;
}

DataForm::Private::~Private()
{
}

QString DataForm::Private::typeToString(Type type)
{
    switch ( type ) {
        case Form:
            return "form";
        case Submit:
            return "submit";
        case Cancel:
            return "cancel";
        case Result:
            return "result";
        case Invalid:
        default:
            return QString();
    }
}

DataForm::Type DataForm::Private::stringToType(const QString& type)
{
    if ( type == "form" ) {
        return Form;
    }
    if ( type == "submit" ) {
        return Submit;
    }
    if ( type == "cancel" ) {
        return Cancel;
    }
    if ( type == "result" ) {
        return Result;
    }
    return Invalid;
}

/**
 * Constructs and empty DataForm object.
 */
DataForm::DataForm()
    : d(new Private)
{
}

/**
 * Makes a deep copy of @a other.
 */
DataForm::DataForm(const DataForm& other)
    : d(other.d)
{
}

/**
 * Destroys DataForm object.
 */
DataForm::~DataForm()
{
}

/**
 * Assigns @a other data-form to this data-form object.
 */
DataForm& DataForm::operator=(const DataForm& other)
{
    d = other.d;
    return *this;
}

bool DataForm::isEmpty() const
{
    if ( d->fields.isEmpty() && d->items.isEmpty() && d->reported.isEmpty() && d->instructions.isEmpty() && d->title.isEmpty() ) {
        return true;
    }
    return false;
}

bool DataForm::isValid() const
{
    if ( isEmpty() || d->type == Invalid ) {
        return false;
    }
    return true;
}

/**
 * Constructs DataForm object from @a form dom-element. @a form should be dom-element which represent the form.
 */
DataForm DataForm::fromDomElement(const QDomElement& form)
{
    if ( form.namespaceURI() != NS_DATA_FORMS ) {
        qWarning("Form is not jabber:x:data");
    }

    DataForm dataForm;

    dataForm.d->type = Private::stringToType( form.attribute("type") );

    QDomNodeList childs = form.childNodes();
    for (int i = 0; i < childs.count(); ++i) {
        QDomNode node = childs.item(i);

        if ( !node.isElement() ) {
            continue;
        }

        if ( node.nodeName() == "field" ) {
            dataForm.d->fields << Field::fromDomElement( node.toElement() );
            continue;
        }
        if ( node.nodeName() == "item" ) {
            dataForm.d->items << Item::fromDomElement( node.toElement() );
            continue;
        }
        if ( node.nodeName() == "reported" ) {
            QDomNodeList reportedFields = node.childNodes();
            for (int j = 0; j < reportedFields.count(); ++j) {
                QDomNode rnode = reportedFields.item(j);
                if ( rnode.nodeName() != "field" ) {
                    qWarning( "Ignoring unknown field in <reported>: %s", qPrintable( rnode.nodeName() ) );
                    continue;
                }
                dataForm.d->reported << Field::fromDomElement( rnode.toElement() );
            }
            continue;
        }
        if ( node.nodeName() == "title" ) {
            dataForm.d->title = node.toElement().text();
            continue;
        }
        if ( node.nodeName() == "instructions" ) {
            dataForm.d->instructions = node.toElement().text();
            continue;
        }
        qWarning( "unknown element: %s", qPrintable( node.nodeName() ) );
    }

    return dataForm;
}

/**
 * Appends data-form as a child element to @a element.
 */
void DataForm::toDomElement(QDomElement& element) const
{
    QDomDocument doc = element.ownerDocument();

    QDomElement eForm = doc.createElementNS(NS_DATA_FORMS, "x");
    element.appendChild(eForm);

    eForm.setAttribute("type", Private::typeToString(d->type) );

    if ( !d->title.isEmpty() ) {
        QDomElement eTitle = doc.createElement("title");
        QDomText eTitleText = doc.createTextNode(d->title);

        eForm.appendChild(eTitle);
        eTitle.appendChild(eTitleText);
    }

    if ( !d->instructions.isEmpty() ) {
        QDomElement eInstructions = doc.createElement("instructions");
        QDomText eInstructionsText = doc.createTextNode(d->instructions);

        eForm.appendChild(eInstructions);
        eInstructions.appendChild(eInstructionsText);
    }

    if ( !d->reported.isEmpty() ) {
        QDomElement eReported = doc.createElement("reported");
        eForm.appendChild(eReported);

        QListIterator<Field> ri(d->reported);
        while ( ri.hasNext() ) {
            ri.next().toDomElement(eReported);
        }
    }

    if ( !d->items.isEmpty() ) {
        QListIterator<Item> ii(d->items);
        while ( ii.hasNext() ) {
            ii.next().toDomElement(eForm);
        }
    }

    if ( !d->fields.isEmpty() ) {
        QListIterator<Field> fi(d->fields);
        while ( fi.hasNext() ) {
            fi.next().toDomElement(eForm);
        }
    }
}

/**
 * Adds a @a field to data-form.
 */
void DataForm::addField(const Field& field)
{
    d->fields << field;
}

/**
 * Adds an @a item to data-form.
 */
void DataForm::addItem(const Item& item)
{
    d->items << item;
}

/**
 * Adds a @a field to data-form.
 */
DataForm& DataForm::operator<<(const Field& field)
{
    d->fields << field;
    return *this;
}

/**
 * Adds an @a item to data-form.
 */
DataForm& DataForm::operator<<(const Item& item)
{
    d->items << item;
    return *this;
}

/**
 * Adds a @a field to data-form \<reported/\> element.
 */
void DataForm::addReportedField(const Field& field)
{
    d->reported << field;
}

/**
 * Returns list of fields contained in the data-form.
 */
QList<DataForm::Field> DataForm::fields() const
{
    return d->fields;
}

/**
 * Returns list of items contained in the data-form.
 */
QList<DataForm::Item> DataForm::items() const
{
    return d->items;
}

/**
 * Returns list of \<reported/\> fields of the data-form.
 */
QList<DataForm::Field> DataForm::reportedFields() const
{
    return d->reported;
}

/**
 * Sets \<reported/\> field-list to @a reported.
 */
void DataForm::setReportedFields(const QList<Field>& reported)
{
    d->reported = reported;
}

/**
 * Sets \<reported/\> fields to single field @a field
 * @overload
 */
void DataForm::setReportedFields(const Field& field)
{
    d->reported.clear();
    d->reported << field;
}

/**
 * Returns data-form type.
 */
DataForm::Type DataForm::type() const
{
    return d->type;
}

/**
 * Returns data-form title string
 */
QString DataForm::title() const
{
    return d->title;
}

/**
 * Returns data-form instructions string
 */
QString DataForm::instructions() const
{
    return d->instructions;
}

/**
 * Sets data-form type to @a type.
 */
void DataForm::setType(Type type)
{
    d->type = type;
}

/**
 * Sets data-form title string to @a title
 */
void DataForm::setTitle(const QString& title)
{
    d->title = title;
}

/**
 * Sets data-form instructions string to @a instructions
 */
void DataForm::setInstructions(const QString& instructions)
{
    d->instructions = instructions;
}

DataForm::Field DataForm::fieldByName(const QString& name)
{
    QListIterator<Field> fi(d->fields);
    while ( fi.hasNext() ) {
        if ( fi.peekNext().name() == name ) {
            return fi.peekNext();
        }
        fi.next();
    }
    return Field();
}

/**
 * @enum DataForm::Type
 * Describes data-form type.
 */

/**
 * @var DataForm::Form
 * The form-processing entity is asking the form-submitting entity to complete a form.
 */

/**
 * @var DataForm::Submit
 * The form-submitting entity is submitting data to the form-processing entity.
 * The submission MAY include fields that were not provided in the empty form, but the form-processing entity MUST ignore any fields that it does not understand.
 */

/**
 * @var DataForm::Cancel
 * The form-submitting entity has cancelled submission of data to the form-processing entity.
 */

/**
 * @var DataForm::Result
 * The form-processing entity is returning data (e.g., search results) to the form-submitting entity, or the data is a generic data set.
 */

/* ***************************************************************************
 * DataForm::Field
 *************************************************************************** */

class DataForm::Field::Private : public QSharedData
{
    public:
        Private();
        Private(const Private& other);
        virtual ~Private();

        static QString typeToString(FieldType type);
        static FieldType stringToType(const QString& type);

        bool required;
        FieldType type;

        QString label, name;
        QString desc;

        QStringList values;
        /* label-value options hash */
        QHash<QString,QString> options;
};

DataForm::Field::Private::Private()
    : QSharedData()
{
    required = false;
    type = TextSingle;
}

DataForm::Field::Private::Private(const Private& other)
    : QSharedData(other)
{
    required = other.required;
    type = other.type;

    label = other.label;
    name = other.name;
    desc = other.desc;

    values = other.values;
}

DataForm::Field::Private::~Private()
{
}

QString DataForm::Field::Private::typeToString(FieldType type)
{
    switch ( type ) {
        case Boolean:
            return "boolean";
        case Fixed:
            return "fixed";
        case Hidden:
            return "hidden";
        case JidMulti:
            return "jid-multi";
        case JidSingle:
            return "jid-single";
        case ListMulti:
            return "list-multi";
        case ListSingle:
            return "list-single";
        case TextMulti:
            return "text-multi";
        case TextPrivate:
            return "text-private";
        case TextSingle:
            return "text-single";
        case Invalid:
        default:
            return QString();
    }
}

DataForm::Field::FieldType DataForm::Field::Private::stringToType(const QString& type)
{
    if ( type == "boolean" ) {
        return Boolean;
    }
    if ( type == "fixed" ) {
        return Fixed;
    }
    if ( type == "hidden" ) {
        return Hidden;
    }
    if ( type == "jid-multi" ) {
        return JidMulti;
    }
    if ( type == "jid-single" ) {
        return JidSingle;
    }
    if ( type == "list-multi" ) {
        return ListMulti;
    }
    if ( type == "list-single" ) {
        return ListSingle;
    }
    if ( type == "text-multi" ) {
        return TextMulti;
    }
    if ( type == "text-private" ) {
        return TextPrivate;
    }
    if ( type == "text-single" ) {
        return TextSingle;
    }
    return Invalid;
}

/**
 * Constructs data-form field object.
 */
DataForm::Field::Field()
    : d(new Private)
{
}

/**
 * Constructs a deep copy of @a other.
 */
DataForm::Field::Field(const Field& other)
    : d(other.d)
{
}

/**
 * Constructs data-form field object with given @a type and @a name.
 */
DataForm::Field::Field(const QString& name, FieldType type)
    : d(new Private)
{
    d->type = type;
    d->name = name;
}

/**
 * Constructs data-form field object with given @a type, @a name and @a label.
 */
DataForm::Field::Field(const QString& name, const QString& label, FieldType type)
    : d(new Private)
{
    d->type = type;
    d->name = name;
    d->label = label;
}

/**
 * Destroys data-form field object.
 */
DataForm::Field::~Field()
{
}

/**
 * Constructs data-form field object with @a name, @a value and (optional) @a type parameters.
 */
DataForm::Field DataForm::Field::fromNameValue(const QString& name, const QString& value, FieldType type)
{
    Field fld;

    fld.d->name = name;
    fld.addValue(value);
    fld.d->type = type;

    return fld;
}

/**
 * Constructs data-form field object with @a name, @a label, @a value and (optional) @a type parameters.
 */
DataForm::Field DataForm::Field::fromNameLabelValue(const QString& name, const QString& label, const QString& value, FieldType type)
{
    Field fld;

    fld.d->name = name;
    fld.d->label = label;
    fld.addValue(value);
    fld.d->type = type;

    return fld;
}

/**
 * Constructs a data-form field object from @a field (\<field/\>) xml element.
 */
DataForm::Field DataForm::Field::fromDomElement(const QDomElement& field)
{
    Field fld;

    fld.d->type = Private::stringToType( field.attribute("type", "text-single") );
    fld.d->name = field.attribute("var");
    fld.d->label = field.attribute("label");

    QDomNodeList options = field.elementsByTagName("option");
    for (uint i = 0; i < options.length(); ++i) {
        QDomElement eOption = options.item(i).toElement();
        QString option = eOption.attribute("label");
        QString value = eOption.firstChildElement("value").text().trimmed();

        fld.d->options.insert(option, value);
    }

    QDomNodeList values = field.elementsByTagName("value");

    for (uint i = 0; i < values.length(); ++i) {
        fld.d->values << values.item(i).toElement().text().trimmed();
    }

    QDomElement eDesc = field.firstChildElement("desc");
    if ( !eDesc.isNull() ) {
        fld.d->desc = eDesc.text().trimmed();
    }

    if ( !field.firstChildElement("required").isNull() ) {
        fld.d->required = true;
    }

    return fld;
}

/**
 * Adds a child \<field/\> element to @a element.
 */
void DataForm::Field::toDomElement(QDomElement& element) const
{
    QDomDocument doc = element.ownerDocument();

    QDomElement eField = doc.createElement("field");
    element.appendChild(eField);

    eField.setAttribute("type", Private::typeToString(d->type) );

    if ( !d->name.isEmpty() ) {
        eField.setAttribute("var", d->name);
    }

    if ( !d->label.isEmpty() ) {
        eField.setAttribute("label", d->label);
    }

    if ( !d->options.isEmpty() ) {
        QHashIterator<QString, QString> oi(d->options);
        while ( oi.hasNext() ) {
            oi.next();
            QDomElement eOption = doc.createElement("option");
            eField.appendChild(eOption);
            eOption.setAttribute( "label", oi.key() );

            QDomElement eValue = doc.createElement("value");
            eOption.appendChild(eValue);

            QDomText eValueText = doc.createTextNode( oi.value() );
            eValue.appendChild(eValueText);
        }
    }

    if ( !d->values.isEmpty() ) {
        QStringListIterator vi(d->values);
        while ( vi.hasNext() ) {
            QDomElement eValue = doc.createElement("value");
            eField.appendChild(eValue);

            QDomText eValueText = doc.createTextNode( vi.next() );
            eValue.appendChild(eValueText);
        }
    }

    if ( !d->desc.isEmpty() ) {
        QDomElement eDesc = doc.createElement("desc");
        eField.appendChild(eDesc);

        QDomText eDescText = doc.createTextNode(d->desc);
        eDesc.appendChild(eDescText);
    }

    if ( d->required ) {
        eField.appendChild( doc.createElement("required") );
    }
}

/**
 * Adds an option to the field.
 */
void DataForm::Field::addOption(const QString& optionLabel, const QString& value)
{
    d->options.insert(optionLabel, value);
}

/**
 * Adds a value to the field.
 */
void DataForm::Field::addValue(const QString& value)
{
    d->values << value;
}

QStringList DataForm::Field::values() const
{
    return d->values;
}

/**
 * Returns field type.
 */
DataForm::Field::FieldType DataForm::Field::type() const
{
    return d->type;
}

/**
 * Sets field type to @a type.
 */
void DataForm::Field::setType(FieldType type)
{
    d->type = type;
}

/**
 * Returns true if field is a required field.
 */
bool DataForm::Field::isRequred() const
{
    return d->required;
}

/**
 * Marks field as required if @a required is true.
 */
void DataForm::Field::setRequired(bool required)
{
    d->required = required;
}

/**
 * Returns field label string.
 */
QString DataForm::Field::label() const
{
    return d->label;
}

/**
 * Sets field label string to @a label.
 */
void DataForm::Field::setLabel(const QString& label)
{
    d->label = label;
}

/**
 * Returns field name string.
 */
QString DataForm::Field::name() const
{
    return d->name;
}

/**
 * Sets field name string ('var' attribute) to @a name.
 */
void DataForm::Field::setName(const QString& name)
{
    d->name = name;
}

/**
 * Returns field desc string.
 */
QString DataForm::Field::desc() const
{
    return d->desc;
}

/**
 * Sets field desc string to @a string.
 */
void DataForm::Field::setDesc(const QString& desc)
{
    d->desc = desc;
}

/**
 * @enum DataForm::Field::FieldType
 * Represents data "types" that are commonly exchanged between Jabber/XMPP entities.
 */

/**
 * @var DataForm::Field::Boolean
 * The field enables an entity to gather or provide an either-or choice between two options. The default value is "false".
 */

/**
 * @var DataForm::Field::Fixed
 * The field is intended for data description (e.g., human-readable text such as "section" headers) rather than data gathering or provision.
 * The \<value/\> child SHOULD NOT contain newlines (the \n and \r characters);
 * instead an application SHOULD generate multiple fixed fields, each with one \<value/\> child.
 */

/**
 * @var DataForm::Field::Hidden
 * The field is not shown to the form-submitting entity, but instead is returned with the form.
 * The form-submitting entity SHOULD NOT modify the value of a hidden field, but MAY do so if such behavior is defined for the "using protocol".
 */

/**
 * @var DataForm::Field::JidMulti
 * The field enables an entity to gather or provide multiple Jabber IDs.
 * Each provided JID SHOULD be unique (as determined by comparison that includes application of the Nodeprep, Nameprep, and Resourceprep profiles
 * of Stringprep as specified in XMPP Core), and duplicate JIDs MUST be ignored.
 */

/**
 * @var DataForm::Field::JidSingle
 * The field enables an entity to gather or provide a single Jabber ID.
 */

/**
 * @var DataForm::Field::ListMulti
 * The field enables an entity to gather or provide one or more options from among many.
 * A form-submitting entity chooses one or more items from among the options presented by the form-processing entity and MUST NOT insert new options.
 * The form-submitting entity MUST NOT modify the order of items as received from the form-processing entity, since the order of items MAY be significant.
 */

/**
 * @var DataForm::Field::ListSingle
 * The field enables an entity to gather or provide one option from among many.
 * A form-submitting entity chooses one item from among the options presented by the form-processing entity and MUST NOT insert new options.
 */

/**
 * @var DataForm::Field::TextMulti
 * The field enables an entity to gather or provide multiple lines of text.
 */

/**
 * @var DataForm::Field::TextPrivate
 * The field enables an entity to gather or provide a single line or word of text, which shall be obscured in an interface (e.g., with multiple instances of the asterisk character).
 */

/**
 * @var DataForm::Field::TextSingle
 * The field enables an entity to gather or provide a single line or word of text, which may be shown in an interface.
 * This field type is the default and MUST be assumed if a form-submitting entity receives a field type it does not understand.
 */

/* ***************************************************************************
 * DataForm::Item
 *************************************************************************** */

class DataForm::Item::Private : public QSharedData
{
    public:
        Private();
        Private(const Private& other);
        virtual ~Private();

        QList<Field> fields;
};

DataForm::Item::Private::Private()
    : QSharedData()
{
}

DataForm::Item::Private::Private(const Private& other)
    : QSharedData(other)
{
    fields = other.fields;
}

DataForm::Item::Private::~Private()
{
}

/**
 * Constructs data-form item object.
 */
DataForm::Item::Item()
    : d(new Private)
{
}

/**
 * Constructs a deep copy of @a other.
 */
DataForm::Item::Item(const Item& other)
    : d(other.d)
{
}

/**
 * Destroys data-form item object.
 */
DataForm::Item::~Item()
{
}

/**
 * Constructs data-form item object from \<item/\> xml-element @a item.
 */
DataForm::Item DataForm::Item::fromDomElement(const QDomElement& item)
{
    Item dfItem;

    QDomNodeList childs = item.childNodes();
    for (int i = 0; i < childs.size(); ++i) {
        QDomElement eField = childs.item(i).toElement();
        if ( eField.tagName() != "field" ) {
            qWarning( "DataForm::Item: item contains unknown tag called '%s'", qPrintable( eField.tagName() ) );
            continue;
        }
        dfItem.d->fields << Field::fromDomElement(eField);
    }

    return dfItem;
}

/**
 * Adds a child \<item/\> element with fields to @a element.
 * @note If item has no fields, this method does nothing.
 */
void DataForm::Item::toDomElement(QDomElement& element) const
{
    if ( d->fields.isEmpty() ) {
        qWarning("DataForm::Item: item has no fields.. ignoring..");
        return;
    }

    QDomElement eItem = element.ownerDocument().createElement("item");
    element.appendChild(eItem);

    QListIterator<Field> fi(d->fields);
    while ( fi.hasNext() ) {
        fi.next().toDomElement(eItem);
    }
}

/**
 * Adds a @a field to item's field-list
 */
void DataForm::Item::addField(const DataForm::Field& field)
{
    d->fields << field;
}

/**
 * Adds a @a field to item's field-list
 */
DataForm::Item& DataForm::Item::operator<<(const DataForm::Field& field)
{
    d->fields << field;
    return *this;
}

/**
 * Returns list of fields contained in this item.
 */
QList<DataForm::Field> DataForm::Item::fields() const
{
    return d->fields;
}

/**
 * Sets item field-list to @a flist.
 */
void DataForm::Item::setFields(const QList<DataForm::Field>& flist)
{
    d->fields = flist;
}


}

// vim:ts=4:sw=4:nowrap:et
