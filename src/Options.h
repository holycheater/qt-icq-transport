/*
 * Options.h - command-line and file options reader/parser
 * Copyright (C) 2008  Alexander Saltykov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <QHash>

class Options
{
    public:
        Options();
        virtual ~Options();

        void parseCommandLine();

        QString getOption(const QString& name) const;
        bool hasOption(const QString& name) const;
    private:
        void readXmlFile(const QString& file);
        void setOption(const QString& option, const QString& value, bool overwrite = false);
        static void printUsage();
        QHash<QString, QString> m_options;
};

// vim:et:ts=4:sw=4:nowrap
#endif /* OPTIONS_H_ */
