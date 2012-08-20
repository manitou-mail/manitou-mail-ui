/* Copyright (C) 2004-2012 Daniel Verite

   This file is part of Manitou-Mail (see http://www.manitou-mail.org)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 as
   published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <QString>
#include <map>

class app_config
{
public:
  app_config() {}
  virtual ~app_config() {}
struct ltstr
{
  bool operator()(const QString s1, QString s2) const
  {
    return strcmp(s1.toLatin1(), s2.toLatin1()) < 0;
  }
};
  void apply();
  void set_name (const QString name) {
    m_name = name;
  }
  const QString name() const {
    return m_name;
  }
  // fetch the contents from the db
  bool init();

  // gets value for a numeric entry
  int get_number(const QString conf_key) const;

  // gets value for a boolean entry
  bool get_bool(const QString conf_key) const;
  // gets value for a boolean entry, with a default
  bool get_bool(const QString conf_key, bool default_val) const;

  // gets value for a string entry
  const QString get_string(const QString conf_key) const;

  // adds or replace a numeric entry
  void set_number(const QString conf_key, int value);

  // adds or replace a boolean entry
  void set_bool(const QString conf_key, bool value);

  // adds or replace a string entry
  void set_string(const QString conf_key, const QString& value);
  // check if an entry exists or not
  bool exists(const QString conf_key);
  // dump to stdout
  void dump();

  // remove an entry
  void remove(const QString conf_key);

  // save to the database the set of key/values whose name start with 'prefix'
  // (or all keys if 'prefix' is null)
  bool store(const QString key_prefix=QString::null);

  // update into the database all the entries in 'newconf'
  // that are different from 'this'
  bool diff_update(const app_config& newconf);

  // replace our entries by those of newconf that have different values
  // than ours
  void diff_replace(const app_config& newconf);


  // specific function querying the "date_format" key and returning 1
  // for european date format or 2 for US format
  int get_date_format_code();

  Qt::SortOrder get_msgs_sort_order() const;
  static bool get_all_conf_names(QStringList*);

protected:
  // TODO: use QMap instead of std::map and get rid of ltstr()
  const std::map<const QString, QString, ltstr>& map() const {
    return m_mapconf;
  }

  std::map<const QString, QString, ltstr> m_mapconf;
  typedef std::map<const QString, QString, ltstr>::const_iterator const_iter;
private:
  // config's name (to be matched against config.conf_name)
  QString m_name;
  static const char* m_default_values[];
};

extern app_config& get_config();

#endif
