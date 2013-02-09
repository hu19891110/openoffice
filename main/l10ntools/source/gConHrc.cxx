/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/
#include "gConHrc.hxx"




/*****************************************************************************
 *************************   G C O N H R C . C X X   *************************
 *****************************************************************************
 * This is the conversion for .hrc, it uses lex to analyze
 *****************************************************************************/



/*****************************   G L O B A L S   *****************************/
convert_hrc_impl * convert_hrc::mcpImpl;



/************   I N T E R F A C E   I M P L E M E N T A T I O N   ************/
convert_hrc::convert_hrc(const std::string& srSourceFile, l10nMem& crMemory, bool brVerbose)
                                : convert_gen(srSourceFile, crMemory, brVerbose) 
                            {mcpImpl = new convert_hrc_impl(srSourceFile, crMemory, brVerbose);}
convert_hrc::~convert_hrc() {delete mcpImpl;}
void convert_hrc::extract() {mcpImpl->extract();}
void convert_hrc::insert()  {mcpImpl->insert();}



/**********************   I M P L E M E N T A T I O N   **********************/
convert_hrc_impl::convert_hrc_impl(const std::string& srSourceFile, l10nMem& crMemory, bool brVerbose)
                                  : convert_gen(srSourceFile, crMemory, brVerbose)
{
}



/**********************   I M P L E M E N T A T I O N   **********************/
convert_hrc_impl::~convert_hrc_impl()
{
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::extract()
{
  mbMergeMode = false;

  // run lex parser and build token tree
  runLex();
}



/**********************   I M P L E M E N T A T I O N   **********************/
void convert_hrc_impl::insert()
{
  mbMergeMode = true;

  // run lex parser and build token tree
  runLex();
}
