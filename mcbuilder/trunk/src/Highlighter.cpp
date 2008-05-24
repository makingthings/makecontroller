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

Highlighter::Highlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
	HighlightingRule rule;
	
	keywordFormat.setForeground(Qt::darkBlue);
	keywordFormat.setFontWeight(QFont::Bold);
	QStringList keywordPatterns;
	keywordPatterns << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b"
									<< "\\bdouble\\b" << "\\benum\\b" << "\\bexplicit\\b"
									<< "\\bfriend\\b" << "\\binline\\b" << "\\bint\\b"
									<< "\\blong\\b" << "\\bnamespace\\b" << "\\boperator\\b"
									<< "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b"
									<< "\\bshort\\b" << "\\bsignals\\b" << "\\bsigned\\b"
									<< "\\bslots\\b" << "\\bstatic\\b" << "\\bstruct\\b"
									<< "\\btemplate\\b" << "\\btypedef\\b" << "\\btypename\\b"
									<< "\\bunion\\b" << "\\bunsigned\\b" << "\\bvirtual\\b"
									<< "\\bvoid\\b" << "\\bvolatile\\b";
	foreach (QString pattern, keywordPatterns) {
		rule.pattern = QRegExp(pattern);
		rule.format = keywordFormat;
		highlightingRules.append(rule);
	}
	
	singleLineCommentFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("//[^\n]*");
	rule.format = singleLineCommentFormat;
	highlightingRules.append(rule);
	
	multiLineCommentFormat.setForeground(Qt::red);
	
	quotationFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\".*\"");
	rule.format = quotationFormat;
	highlightingRules.append(rule);
	
	functionFormat.setFontItalic(true);
	functionFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
	rule.format = functionFormat;
	highlightingRules.append(rule);
	
	commentStartExpression = QRegExp("/\\*");
	commentEndExpression = QRegExp("\\*/");
}

/*
  This is called back by the rich text engine when a line of text has changed.
  
*/
void Highlighter::highlightBlock(const QString &text)
{
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
	setCurrentBlockState(NOT_IN_COMMENT);
	
	int startIndex = 0;
	if (previousBlockState() != 1)
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



