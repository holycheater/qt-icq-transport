/*
 * main.cpp - application entry point
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

#include "TransportMain.h"
#include <QTextCodec>

int main(int argc, char **argv)
{
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName("UTF-8") );
    TransportMain app(argc, argv);
    return app.exec();
}

// vim:et:ts=4:sw=4:nowrap
