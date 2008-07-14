/*********************************************************************************

 Copyright 2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#include "Highlighter.h"

#define IN_COMMENT 1
#define NOT_IN_COMMENT 0

/*
  Highlighter provides syntax highliting to the editor via a QSyntaxHighlighter interface.
  Specify the expressions we want to match via RegEx, and apply some
  formatting to them.  
  
  TODO - set up Preferences UI to let user select colors for highlighting
*/
Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
	HighlightingRule rule;
	
	keywordFormat.setForeground(QColor("#C50096"));
	QStringList keywordPatterns;
	keywordPatterns << "\\bchar\\b" << "\\bconst\\b" << "\\bwhile\\b"
									<< "\\bdouble\\b" << "\\benum\\b" << "\\bfor\\b"
									<< "\\binline\\b" << "\\bint\\b" << "\\btrue\\b"
									<< "\\blong\\b" << "\\boperator\\b" << "\\bfalse\\b"
                  << "\\belse\\b" << "\\bthis\\b" << "\\breturn\\b"
									<< "\\bswitch\\b" << "\\bcase\\b" << "\\bbreak\\b"
									<< "\\bshort\\b" << "\\bsigned\\b" << "\\bstatic\\b" 
									<< "\\btypedef\\b" << "\\btypename\\b" << "\\bif\\b"
									<< "\\bunion\\b" << "\\bunsigned\\b"
									<< "\\bvoid\\b" << "\\bvolatile\\b" << "\\bstruct\\b";
                  
	foreach (QString pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}
	
  quotationFormat.setForeground(QColor("#E20000"));
	rule.pattern = QRegExp("\".*\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);
  
  preprocFormat.setForeground(QColor("#6E3719"));
	rule.pattern = QRegExp("^#[^\n]*");
	rule.format = preprocFormat;
	highlightingRules.append(rule);
  
  digitFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("[0-9]");
	rule.format = digitFormat;
	highlightingRules.append(rule);
	
  singleLineCommentFormat.setForeground(QColor("#007800"));
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);
	
	multiLineCommentFormat.setForeground(QColor("#007800"));
	
	commentStartExpression = QRegExp("/\\*");
	commentEndExpression = QRegExp("\\*/");
}

/*
  This is called back by the rich text engine when a line of text has changed.
*/
void Highlighter::highlightBlock(const QString &text)
{
  // search the block for each of our highlight rules, and if we find a match
  // apply the format for that rule
  foreach (HighlightingRule rule, highlightingRules)
	{
		QRegExp expression(rule.pattern);
		int index = text.indexOf(expression);
		while (index >= 0)
		{
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = text.indexOf(expression, index + length);
		}
	}
  
  // now deal with the case of multi-line comments that might span more than one block
	setCurrentBlockState(NOT_IN_COMMENT);
	
	int startIndex = 0;
	if (previousBlockState() != IN_COMMENT)
		startIndex = text.indexOf(commentStartExpression);
	
	while (startIndex >= 0)
	{
		int endIndex = text.indexOf(commentEndExpression, startIndex);
		int commentLength;
		if (endIndex == -1)
		{
			setCurrentBlockState(IN_COMMENT);
			commentLength = text.length() - startIndex;
		}
		else
			commentLength = endIndex - startIndex + commentEndExpression.matchedLength();
		setFormat(startIndex, commentLength, multiLineCommentFormat);
		startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
	}
}



