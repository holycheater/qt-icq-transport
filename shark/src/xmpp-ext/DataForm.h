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

		static DataForm fromDomElement(const QDomElement& form);
		void toDomElement(QDomElement& element) const;

		void addField(const Field& field);
		void addItem(const Item& item);

		QList<Field> fields() const;
		QList<Item> items() const;

		QList<Field> reportedFields() const;
		void addReportedField(const Field& field);
		void setReportedFields(const QList<Field>& reported);
		void setReportedFields(const Field& field);

		QString title() const;
		QString instructions() const;

		void setTitle(const QString& title);
		void setInstructions(const QString& instructions);
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
		Field(FieldType type, const QString& name);
		Field(FieldType type, const QString& name, const QString& label);
		virtual ~Field();

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
		QList<Field> fields() const;
		void setFields(const QList<Field>& flist);
	private:
		class Private;
		QSharedDataPointer<Private> d;
};


}

#endif /* XMPP_DATA_FORM_H_ */
