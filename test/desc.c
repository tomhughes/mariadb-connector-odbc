/*
  Copyright (c) 2001, 2012, Oracle and/or its affiliates. All rights reserved.
                2013 MontyProgram AB

  The MySQL Connector/ODBC is licensed under the terms of the GPLv2
  <http://www.gnu.org/licenses/old-licenses/gpl-2.0.html>, like most
  MySQL Connectors. There are special exceptions to the terms and
  conditions of the GPLv2 as it is applied to this software, see the
  FLOSS License Exception
  <http://www.mysql.com/about/legal/licensing/foss-exception.html>.
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation; version 2 of the License.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
  for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include "tap.h"

/*
 * Tests for paramset:
 * SQL_ATTR_PARAMSET_SIZE (apd->array_size)
 * SQL_ATTR_PARAM_STATUS_PTR (ipd->array_status_ptr)
 * SQL_ATTR_PARAM_OPERATION_PTR (apd->array_status_ptr)
 * SQL_ATTR_PARAMS_PROCESSED_PTR (apd->rows_processed_ptr)
 */
ODBC_TEST(t_desc_paramset)
{
  SQLULEN       parsetsize= 4;
  SQLUSMALLINT  parstatus[4];
  SQLUSMALLINT  parop[4]; /* operation */
  SQLULEN       pardone; /* processed */
  SQLHANDLE     ipd, apd;
  SQLINTEGER    params1[4];
  SQLINTEGER    params2[4];

  parop[0]= SQL_PARAM_PROCEED;
  parop[1]= SQL_PARAM_IGNORE;
  parop[2]= SQL_PARAM_IGNORE;
  parop[3]= SQL_PARAM_PROCEED;
  params1[0]= 0;
  params1[1]= 1;
  params1[2]= 2;
  params1[3]= 3;
  params2[0]= 100;
  params2[1]= 101;
  params2[2]= 102;
  params2[3]= 103;

  /* get the descriptors */
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC,
                                &apd, SQL_IS_POINTER, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_PARAM_DESC,
                                &ipd, SQL_IS_POINTER, NULL));

  /* set the fields */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMSET_SIZE,
                                (SQLPOINTER) parsetsize, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_STATUS_PTR,
                                parstatus, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAM_OPERATION_PTR,
                                parop, 0));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_PARAMS_PROCESSED_PTR,
                                &pardone, 0));

  /* verify the fields */
  {
    SQLPOINTER  x_parstatus, x_parop, x_pardone;
    SQLULEN     x_parsetsize;
    
    CHECK_DESC_RC(apd, SQLGetDescField(apd, 0, SQL_DESC_ARRAY_SIZE,
                                 &x_parsetsize, SQL_IS_UINTEGER, NULL));
    CHECK_DESC_RC(ipd, SQLGetDescField(ipd, 0, SQL_DESC_ARRAY_STATUS_PTR,
                                 &x_parstatus, SQL_IS_POINTER, NULL));
    /* todo: wring result: SQL_ATTR_PARAM_OPERATION_PTR */
    CHECK_DESC_RC(apd, SQLGetDescField(apd, 0, SQL_DESC_ARRAY_STATUS_PTR,
                                 &x_parop, SQL_IS_POINTER, NULL));
    /* todo: wrong result: SQL_ATTR_PARAMS_PROCESSEd_PTR */
    CHECK_DESC_RC(ipd, SQLGetDescField(ipd, 0, SQL_DESC_ROWS_PROCESSED_PTR,
                                 &x_pardone, SQL_IS_POINTER, NULL));

    is_num(x_parsetsize, parsetsize);
    FAIL_IF(x_parstatus != parstatus, "x_parstatus != parstatus");
    FAIL_IF(x_parop != parop, "x_parop != parop");
    FAIL_IF(x_pardone != &pardone, "x_pardone != pardone");
  }

  OK_SIMPLE_STMT(Stmt, "DROP TABLE IF EXISTS t_paramset");
  OK_SIMPLE_STMT(Stmt, "create table t_paramset(x int, y int)");
  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, "insert into t_paramset values (?, ?)", SQL_NTS));

  CHECK_STMT_RC(Stmt,
          SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_INTEGER, SQL_C_LONG,
                           0, 0, params1, sizeof(SQLINTEGER), NULL));
  CHECK_STMT_RC(Stmt,
          SQLBindParameter(Stmt, 2, SQL_PARAM_INPUT, SQL_INTEGER, SQL_C_LONG,
                           0, 0, params2, sizeof(SQLINTEGER), NULL));
  /*
  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  */
  /* TODO, finish test and implement */

  return OK;
}


/*
 * Test for errors when setting descriptor fields.
 */
ODBC_TEST(t_desc_set_error)
{
  SQLHANDLE ird, ard;
  SQLPOINTER array_status_ptr= (SQLPOINTER) 0xc0c0;

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_ROW_DESC,
                                &ird, SQL_IS_POINTER, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &ard, SQL_IS_POINTER, NULL));

  /* Test bad header field permissions */
  FAIL_IF(SQLSetDescField(ard, 0, SQL_DESC_ROWS_PROCESSED_PTR,
                                   NULL, SQL_IS_POINTER) != SQL_ERROR, "Error exppected");
  FAIL_IF(check_sqlstate_ex(ard, SQL_HANDLE_DESC, "HY091") != OK, "sqlstate != HY091");

  /* Test the HY016 error received when setting any field on an IRD
   * besides SQL_DESC_ARRAY_STATUS_PTR or SQL_DESC_ROWS_PROCESSED_PTR.
   *
   * Windows intercepts this and returns HY091
   */
  CHECK_DESC_RC(ird, SQLSetDescField(ird, 0, SQL_DESC_ARRAY_STATUS_PTR,
                               array_status_ptr, SQL_IS_POINTER));
  FAIL_IF(SQLSetDescField(ird, 0, SQL_DESC_AUTO_UNIQUE_VALUE,
                                   (SQLPOINTER) 1, SQL_IS_INTEGER) != SQL_ERROR, "Error expected");

  FAIL_IF(check_sqlstate_ex(ird, SQL_HANDLE_DESC, "HY091") != OK, "HY091 expected");

  /* Test invalid field identifier (will be HY016 on ird, HY091 on others) */
  FAIL_IF(SQLSetDescField(ard, 0, 999, NULL, SQL_IS_POINTER) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate_ex(ard, SQL_HANDLE_DESC, "HY091") != OK, "HY091 expected");

  /* Test bad data type (SQLINTEGER cant be assigned to SQLPOINTER) 
  FAIL_IF( SQLSetDescField(ard, 0, SQL_DESC_BIND_OFFSET_PTR,
                                   NULL, SQL_IS_INTEGER) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate_ex(ard, SQL_HANDLE_DESC, "HY015") != OK, "HY015 expected");
  */
  return OK;
}


/*
   Implicit Resetting of COUNT Field with SQLBindCol()
*/
ODBC_TEST(t_sqlbindcol_count_reset)
{
  SQLHANDLE ard;
  SQLINTEGER count;
  SQLCHAR *buf[10];

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &ard, SQL_IS_POINTER, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 0);

  OK_SIMPLE_STMT(Stmt, "select 1,2,3,4,5");

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 0);

  /* bind column 3 -> expand to count = 3 */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, buf, 10, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 3);

  /* unbind column 3 -> contract to count = 0 */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_DEFAULT, NULL, 0, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 0);

  /* bind column 2 -> expand to count = 2 */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 2, SQL_C_CHAR, buf, 10, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 2);

  /* bind column 3 -> expand to count = 3 */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_CHAR, buf, 10, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 3);

  /* unbind column 3 -> contract to count = 2 */
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 3, SQL_C_DEFAULT, NULL, 0, NULL));

  CHECK_DESC_RC(ard, SQLGetDescField(ard, 0, SQL_DESC_COUNT, &count,
                               SQL_IS_INTEGER, NULL));
  is_num(count, 2);

  return OK;
}


/*
  Test that if no type is given to SQLSetDescField(), that the
  correct default is used. See Bug#31720.
*/
ODBC_TEST(t_desc_default_type)
{
  SQLHANDLE ard, apd;
  SQLINTEGER inval= 20, outval= 0;

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *)"select ?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC,
                                &apd, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &ard, 0, NULL));

  CHECK_DESC_RC(apd, SQLSetDescField(apd, 1, SQL_DESC_CONCISE_TYPE,
                               (SQLPOINTER) SQL_C_LONG, 0));
  CHECK_DESC_RC(apd, SQLSetDescField(apd, 1, SQL_DESC_DATA_PTR, &inval, 0));

  CHECK_DESC_RC(ard, SQLSetDescField(ard, 1, SQL_DESC_CONCISE_TYPE,
                               (SQLPOINTER) SQL_C_LONG, 0));
  CHECK_DESC_RC(ard, SQLSetDescField(ard, 1, SQL_DESC_DATA_PTR, &outval, 0));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));

  is_num(outval, inval);

  return OK;
}


/*
  Basic use of explicitly allocated descriptor
*/
ODBC_TEST(t_basic_explicit)
{
  SQLHANDLE expapd;
  SQLINTEGER result;
  SQLINTEGER impparam= 2;
  SQLINTEGER expparam= 999;

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *) "select ?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &result, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &impparam, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  result= 0;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(result, impparam);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* setup a new descriptor */
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expapd));

  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC, expapd, 0));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &expparam, 0, NULL));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  result= 0;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(result, expparam);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  /* free the descriptor, will set apd back to original on hstmt */
  CHECK_DESC_RC(expapd, SQLFreeHandle(SQL_HANDLE_DESC, expapd));

  CHECK_STMT_RC(Stmt, SQLExecute(Stmt));

  result= 0;
  CHECK_STMT_RC(Stmt, SQLFetch(Stmt));
  is_num(result, impparam);
  CHECK_STMT_RC(Stmt, SQLFreeStmt(Stmt, SQL_CLOSE));

  return OK;
}


/*
  Test the various error scenarios possible with
  explicitly allocated descriptors
*/
ODBC_TEST(t_explicit_error)
{
  SQLHANDLE desc1, desc2;
  SQLHANDLE expapd;
  SQLHANDLE hstmt2;

  /* TODO using an exp from a different dbc */

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt2));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &desc1, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(hstmt2, SQL_ATTR_APP_ROW_DESC,
                                 &desc2, 0, NULL));

  /* can't set implicit ard from a different statement */
  FAIL_IF(SQLSetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                    desc2, 0) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate(Stmt, "HY017") != OK, "HY017 expected");

  /* can set it to the same statement */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC, desc1, 0));

  /* can't set implementation descriptors */
  FAIL_IF(SQLSetStmtAttr(Stmt, SQL_ATTR_IMP_ROW_DESC,
                                    desc1, 0) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate(Stmt, "HY024") != OK &&
          check_sqlstate(Stmt, "HY017") != OK, "Expected HY024 or HY017");

  /*
    can't free implicit descriptors
    This crashes unixODBC 2.2.11, as it improperly frees the descriptor,
    and again tries to when freeing the statement handle.
  */
  
  FAIL_IF(SQLFreeHandle(SQL_HANDLE_DESC, desc1) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate_ex(desc1, SQL_HANDLE_DESC, "HY017") != OK, "expected OK");
  
  /* can't set apd as ard (and vice-versa) */
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expapd));
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(Stmt, SQL_ATTR_APP_PARAM_DESC,
                                expapd, 0)); /* this makes expapd an apd */

  FAIL_IF(SQLSetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                    expapd, 0) != SQL_ERROR, "Error expected");
  FAIL_IF(check_sqlstate(Stmt, "HY024") != OK, "Expected HY024");

  /*
    this exposes a bug in unixODBC (2.2.12 and current as of 2007-12-14).
    Even though the above call failed, unixODBC saved this value internally
    and returns it. desc1 should *not* be the same as the explicit apd
  */
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &desc1, 0, NULL));
  diag("explicit apd: %x, stmt's ard: %x", expapd, desc1);

  return OK;
}


/*
  Test that statements are handled correctly when freeing an
  explicit descriptor associated with multiple statements.
*/
ODBC_TEST(t_mult_stmt_free)
{
#define mult_count 3
  SQLHANDLE expard, expapd;
#ifndef _WIN32
  SQLHANDLE desc;
#endif
  SQLHANDLE stmt[mult_count];
  SQLINTEGER i;
  SQLINTEGER imp_params[mult_count];
  SQLINTEGER imp_results[mult_count];
  SQLINTEGER exp_param;
  SQLINTEGER exp_result;

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expapd));
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expard));

  for (i= 0; i < mult_count; ++i)
  {
    CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &stmt[i]));
    /* we bind these now, but use at the end */
    imp_params[i]= 900 + i;
    CHECK_STMT_RC(stmt[i], SQLBindParameter(stmt[i], 1, SQL_PARAM_INPUT, SQL_INTEGER,
                                      SQL_C_LONG, 0, 0, &imp_params[i], 0, NULL));
    CHECK_STMT_RC(stmt[i], SQLBindCol(stmt[i], 1, SQL_C_LONG, &imp_results[i], 0, NULL));
    CHECK_STMT_RC(stmt[i], SQLSetStmtAttr(stmt[i], SQL_ATTR_APP_ROW_DESC, expard, 0));
    CHECK_STMT_RC(stmt[i], SQLSetStmtAttr(stmt[i], SQL_ATTR_APP_PARAM_DESC, expapd, 0));
  }

  /* this will work for all */
  CHECK_STMT_RC(stmt[0], SQLBindParameter(stmt[0], 1, SQL_PARAM_INPUT, SQL_INTEGER,
                                    SQL_C_LONG, 0, 0, &exp_param, 0, NULL));
  CHECK_STMT_RC(stmt[0], SQLBindCol(stmt[0], 1, SQL_C_LONG, &exp_result, 0, NULL));

  /* check that the explicit ard and apd are working */
  for (i= 0; i < mult_count; ++i)
  {
    exp_param= 200 + i;
    CHECK_STMT_RC(stmt[i], SQLExecDirect(stmt[i], (SQLCHAR *)"select ?", SQL_NTS));
    CHECK_STMT_RC(stmt[i], SQLFetch(stmt[i]));
    is_num(exp_result, exp_param);
    CHECK_STMT_RC(stmt[i], SQLFreeStmt(stmt[i], SQL_CLOSE));
  }

  /*
    Windows ODBC DM has a bug that crashes when using a statement
    after free-ing an explicitly allocated that was associated with
    it. (This exact test was run against SQL Server and crashed in
    the exact same spot.) Tested on Windows 2003 x86 and
    Windows XP x64 both w/MDAC 2.8.
   */
#ifndef _WIN32
  /*
    now free the explicit apd+ard and the stmts should go back to
    their implicit descriptors
  */
  CHECK_DESC_RC(expard, SQLFreeHandle(SQL_HANDLE_DESC, expard));
  CHECK_DESC_RC(expapd, SQLFreeHandle(SQL_HANDLE_DESC, expapd));

  /* check that the original values worked */
  for (i= 0; i < mult_count; ++i)
  {
    CHECK_STMT_RC(stmt[i], SQLExecDirect(stmt[i], (SQLCHAR *)"select ?", SQL_NTS));
    CHECK_STMT_RC(stmt[i], SQLFetch(stmt[i]));
    CHECK_STMT_RC(stmt[i], SQLFreeStmt(stmt[i], SQL_CLOSE));
  }

  for (i= 0; i < mult_count; ++i)
  {
    is_num(imp_results[i], imp_params[i]);
  }

  /*
    bug in unixODBC - it still returns the explicit descriptor
    These should *not* be the same
  */
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(stmt[0], SQL_ATTR_APP_ROW_DESC,
                               &desc, SQL_IS_POINTER, NULL));
  printf("# explicit ard = %p, stmt[0]'s implicit ard = %p\n", expard, desc);
#endif

  return OK;
#undef mult_count
}


/*
  Test that when we set a stmt's ard from an explicit descriptor to
  null, it uses the implicit descriptor again. Also the statement
  will disassociate itself from the explicit descriptor.
*/
ODBC_TEST(t_set_null_use_implicit)
{
  SQLHANDLE expard, hstmt1;
  SQLINTEGER imp_result= 0, exp_result= 0;

   /*
    we use a separate statement handle to test that it correctly
    disassociates itself from the descriptor
  */
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt1));

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expard));

  /* this affects the implicit ard */
  CHECK_STMT_RC(Stmt, SQLBindCol(hstmt1, 1, SQL_C_LONG, &imp_result, 0, NULL));

  /* set the explicit ard */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(hstmt1, SQL_ATTR_APP_ROW_DESC, expard, 0));

  /* this affects the expard */
  CHECK_STMT_RC(Stmt, SQLBindCol(hstmt1, 1, SQL_C_LONG, &exp_result, 0, NULL));

  /* set it to null, getting rid of the expard */
  CHECK_STMT_RC(Stmt, SQLSetStmtAttr(hstmt1, SQL_ATTR_APP_ROW_DESC,
                                SQL_NULL_HANDLE, 0));

  OK_SIMPLE_STMT(hstmt1, "select 1");
  CHECK_STMT_RC(Stmt, SQLFetch(hstmt1));

  is_num(exp_result, 0);
  is_num(imp_result, 1);

  CHECK_STMT_RC(Stmt, SQLFreeHandle(SQL_HANDLE_STMT, hstmt1));

  /* if stmt disassociation failed, this will crash */
  CHECK_DESC_RC(expard, SQLFreeHandle(SQL_HANDLE_DESC, expard));

  return OK;
}


/*
  Test free-ing a statement that has an explicitely allocated
  descriptor associated with it. If this test fails, it will
  crash due to the statement not being disassociated with the
  descriptor correctly.
*/
ODBC_TEST(t_free_stmt_with_exp_desc)
{
  SQLHANDLE expard, hstmt1;
  SQLINTEGER imp_result= 0, exp_result= 0;

  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_DESC, Connection, &expard));
  CHECK_DBC_RC(Connection, SQLAllocHandle(SQL_HANDLE_STMT, Connection, &hstmt1));

  /* set the explicit ard */
  CHECK_STMT_RC(hstmt1, SQLSetStmtAttr(hstmt1, SQL_ATTR_APP_ROW_DESC, expard, 0));

  /* free the statement, THEN the descriptors */
  CHECK_STMT_RC(hstmt1, SQLFreeHandle(SQL_HANDLE_STMT, hstmt1));
  CHECK_DESC_RC(expard, SQLFreeHandle(SQL_HANDLE_DESC, expard));

  return OK;
}


/*
  Bug #41081, Unable to retreive null SQL_NUMERIC with ADO.
  It's setting SQL_DESC_PRECISION after SQLBindCol(). We were
  "unbinding" incorrectly, by not only clearing data_ptr, but
  also octet_length_ptr and indicator_ptr.
*/
ODBC_TEST(t_bug41081)
{
  SQLHANDLE ard;
  SQLINTEGER res;
  SQLLEN ind;
  SQLPOINTER data_ptr, octet_length_ptr;
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_APP_ROW_DESC,
                                &ard, SQL_IS_POINTER, NULL));
  OK_SIMPLE_STMT(Stmt, "select 1");
  CHECK_STMT_RC(Stmt, SQLBindCol(Stmt, 1, SQL_C_LONG, &res, 0, &ind));
  /* cause to unbind */
  CHECK_DESC_RC(ard, SQLSetDescField(ard, 1, SQL_DESC_PRECISION, (SQLPOINTER) 10,
                               SQL_IS_SMALLINT));
  /* check proper unbinding */
  CHECK_DESC_RC(ard, SQLGetDescField(ard, 1, SQL_DESC_DATA_PTR,
                               &data_ptr, SQL_IS_POINTER, NULL));
  CHECK_DESC_RC(ard, SQLGetDescField(ard, 1, SQL_DESC_OCTET_LENGTH_PTR,
                               &octet_length_ptr, SQL_IS_POINTER, NULL));
  FAIL_IF(data_ptr != NULL, "data_ptr = NULL expected");
  FAIL_IF(octet_length_ptr != &ind, "octet_length_ptr != &ind");
  return OK;
}


/*
  Bug #44576 - SQL_DESC_DATETIME_INTERVAL_CODE not set on descriptor
*/
ODBC_TEST(t_bug44576)
{
  SQLSMALLINT interval_code;
  SQLSMALLINT concise_type;
  SQLHANDLE ird;
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_ROW_DESC, &ird, 0, NULL));
  OK_SIMPLE_STMT(Stmt, "select cast('2000-10-10' as date)");
  CHECK_DESC_RC(ird, SQLGetDescField(ird, 1, SQL_DESC_CONCISE_TYPE, &concise_type,
                               SQL_IS_SMALLINT, NULL));
  CHECK_DESC_RC(ird, SQLGetDescField(ird, 1, SQL_DESC_DATETIME_INTERVAL_CODE,
                               &interval_code, SQL_IS_SMALLINT, NULL));
  is_num(concise_type, SQL_TYPE_DATE);
  is_num(interval_code, SQL_CODE_DATE);
  return OK;
}


/* If no default database is selected for the connection, call of SQLColumns
   causes error "Unknown database 'null'" */
ODBC_TEST(t_desc_curcatalog)
{
  SQLHANDLE Connection1;
  SQLHSTMT hstmt1;
  SQLCHAR conn_in[512];
  SQLHANDLE ird;

  CHECK_ENV_RC(Env, SQLAllocConnect(Env, &Connection1));

  /* Connecting not specifying default db */
  sprintf((char *)conn_in, "DRIVER=%s;SERVER=%s;UID=%s;PWD=%s;PORT=%u", my_drivername, my_servername,
                              my_uid, my_pwd, my_port);
  
  CHECK_DBC_RC(Connection1, SQLDriverConnect(Connection1, NULL, conn_in, sizeof(conn_in), NULL,
                                 0, NULL,
                                 SQL_DRIVER_NOPROMPT));

  CHECK_DBC_RC(Connection1, SQLAllocStmt(Connection1, &hstmt1));

  OK_SIMPLE_STMT(hstmt1, "select 10 AS no_catalog_column");

  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(hstmt1, SQL_ATTR_IMP_ROW_DESC, &ird, 0, NULL));
  CHECK_DESC_RC(ird, SQLGetDescField(ird, 1, SQL_DESC_CATALOG_NAME, conn_in,
                               sizeof(conn_in), NULL));

  FAIL_IF(conn_in[0]!=0, "expected conn_in = NULL");

  CHECK_DBC_RC(Connection1, SQLFreeStmt(hstmt1, SQL_CLOSE));

  CHECK_DBC_RC(Connection1, SQLDisconnect(Connection1));
  CHECK_DBC_RC(Connection1, SQLFreeConnect(Connection1));


  return OK;
}


ODBC_TEST(t_odbc14)
{
  SQLHANDLE  ipd;
  SQLINTEGER param;

  CHECK_STMT_RC(Stmt, SQLPrepare(Stmt, (SQLCHAR *) "select ?", SQL_NTS));
  CHECK_STMT_RC(Stmt, SQLBindParameter(Stmt, 1, SQL_PARAM_INPUT, SQL_C_LONG,
                                  SQL_INTEGER, 0, 0, &param, 0, NULL));
  CHECK_STMT_RC(Stmt, SQLGetStmtAttr(Stmt, SQL_ATTR_IMP_PARAM_DESC,
                                &ipd, SQL_IS_POINTER, NULL));
  CHECK_DESC_RC(ipd, SQLSetDescField(ipd, 1, SQL_DESC_UNNAMED, (SQLPOINTER) SQL_UNNAMED,
                               SQL_IS_SMALLINT));
  FAIL_IF(SQLSetDescField(ipd, 1, SQL_DESC_UNNAMED, (SQLPOINTER) SQL_NAMED,
                               SQL_IS_SMALLINT) != SQL_ERROR, "Error expected for SQL_NAMED");

  CHECK_SQLSTATE_EX(ipd, SQL_HANDLE_DESC, "HY092");

  return OK;
}


MA_ODBC_TESTS my_tests[]=
{
  {t_desc_paramset,"t_desc_paramset"},
  {t_desc_set_error, "t_desc_set_error"},
  {t_sqlbindcol_count_reset, "t_sqlbindcol_count_reset"},
  {t_desc_default_type, "t_desc_default_type"},
  {t_basic_explicit, "t_basic_explicit"},
  {t_explicit_error, "t_explicit_error"},
  {t_mult_stmt_free, "t_mult_stmt_free"},
  {t_set_null_use_implicit, "t_set_null_use_implicit"},
  {t_free_stmt_with_exp_desc, "t_free_stmt_with_exp_desc"},
  {t_bug41081,"t_bug41081"},
  {t_bug44576, "t_bug44576"},
  {t_desc_curcatalog, "t_desc_curcatalog"},
  {t_odbc14, "t_odbc14"},
  {NULL, NULL}
};

int main(int argc, char **argv)
{
  int tests= sizeof(my_tests)/sizeof(MA_ODBC_TESTS) - 1;
  get_options(argc, argv);
  plan(tests);
  mark_all_tests_normal(my_tests);
  return run_tests(my_tests);
}
