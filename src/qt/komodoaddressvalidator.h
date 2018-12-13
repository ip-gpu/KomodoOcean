// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KOMODO_QT_KOMODOADDRESSVALIDATOR_H
#define KOMODO_QT_KOMODOADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class KomodoAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit KomodoAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Komodo address widget validator, checks for a valid komodo address.
 */
class KomodoAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit KomodoAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // KOMODO_QT_KOMODOADDRESSVALIDATOR_H
