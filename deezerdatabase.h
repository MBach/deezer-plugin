#ifndef DEEZERDATABASE_H
#define DEEZERDATABASE_H

#include "model/sqldatabase.h"

/**
 * \brief       Deezer cache database
 * \author      Matthieu Bachelier
 * \copyright   GNU General Public License v3
 */
class DeezerDatabase : public QSqlDatabase
{
private:
	bool _isEmpty;

public:
	explicit DeezerDatabase();

	inline bool isEmpty() const { return _isEmpty; }

	void extractTo(SqlDatabase *db);
};

#endif // DEEZERDATABASE_H
