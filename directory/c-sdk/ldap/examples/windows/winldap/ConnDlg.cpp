/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 * 
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 * 
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 * 
 * The Original Code is Mozilla Communicator client code.
 * 
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1996-1999
 * the Initial Developer. All Rights Reserved.
 * 
 * Contributor(s):
 * 
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 * 
 * ***** END LICENSE BLOCK ***** */

// ConnDlg.cpp : implementation file
//

#include "stdafx.h"
#include "winldap.h"
#include "ConnDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// ConnDlg dialog


ConnDlg::ConnDlg(CWnd* pParent /*=NULL*/)
	: CDialog(ConnDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(ConnDlg)
	m_dirHost = _T("");
	m_dirPort = 0;
	//}}AFX_DATA_INIT
}


void ConnDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ConnDlg)
	DDX_Text(pDX, IDC_DIR_HOST, m_dirHost);
	DDX_Text(pDX, IDC_DIR_PORT, m_dirPort);
	DDV_MinMaxInt(pDX, m_dirPort, 1, 32000);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ConnDlg, CDialog)
	//{{AFX_MSG_MAP(ConnDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ConnDlg message handlers
