#!/usr/bin/python3
# sh_xgettext
# Arnaldo Carvalho de Melo <acme@conectiva.com.br>
# Wed Mar 10 10:24:35 EST 1999
# Copyright Conectiva Consultoria e Desenvolvimento de Sistemas LTDA
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Changelog
# Mon May 31 1999 Wanderlei Antonio Cavassin <cavassin@conectiva.com>
# * option --initscripts


from sys import argv
import string
import re

s = {}
pattern = re.compile('[ =]\$"')


def xgettext(arq):
    line = 0
    f = open(arq, "r")

    while 1:
        l = f.readline()

        if not l:
            break

        line = line + 1

        if l[0:1] == '#':
            continue
        elif l[0:1] == '\n':
            continue
        else:
            for match in pattern.finditer(l):
                pos = match.start()
                p1 = l.find('"', pos) + 1
                p2 = p1 + 1

                while 1:
                    p2 = l.find('"', p2)
                    if p2 == -1:
                        p2 = p1
                        break
                    if l[p2-1] == '\\':
                        p2 = p2 + 1
                    else:
                        break

                text = l[p1:p2]

                if text in s:
                    s[text].append((arq, line))
                else:
                    s[text] = [(arq, line)]

    f.close()


def print_header():
    print('msgid ""')
    print('msgstr ""')
    print('"Project-Id-Version: PACKAGE VERSION\\n"')
    print('"Report-Msgid-Bugs-To: \\n"')
    print('"POT-Creation-Date: YEAR-MO-DA HO:MI+ZONE\\n"')
    print('"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n"')
    print('"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"')
    print('"Language-Team: LANGUAGE <LL@li.org>\\n"')
    print('"Language: \\n"')
    print('"MIME-Version: 1.0\\n"')
    print('"Content-Type: text/plain; charset=UTF-8\\n"')
    print('"Content-Transfer-Encoding: 8bit\\n"\n')


def print_pot():
    print_header()

    for text in list(s.keys()):
        print('#:', end=' ')

        for p in s[text]:
            print('%s:%d' % p, end=' ')

        if text.find('%') != -1:
            print('\n#, c-format', end=' ')

        print('\nmsgid "' + text + '"')
        print('msgstr ""\n')


def main():
    for a in argv:
        xgettext(a)

    print_pot()

if __name__ == '__main__':
    main()
