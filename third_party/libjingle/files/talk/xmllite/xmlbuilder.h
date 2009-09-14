/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products 
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _xmlbuilder_h_
#define _xmlbuilder_h_

#include <string>
#include "talk/base/scoped_ptr.h"
#include "talk/base/stl_decl.h"
#include "talk/xmllite/xmlparser.h"

#include <expat.h>

namespace buzz {

class XmlElement;
class XmlParseContext;


class XmlBuilder : public XmlParseHandler {
public:
  XmlBuilder();

  static XmlElement * BuildElement(XmlParseContext * pctx,
                                  const char * name, const char ** atts);
  virtual void StartElement(XmlParseContext * pctx,
                            const char * name, const char ** atts);
  virtual void EndElement(XmlParseContext * pctx, const char * name);
  virtual void CharacterData(XmlParseContext * pctx,
                             const char * text, int len);
  virtual void Error(XmlParseContext * pctx, XML_Error);
  virtual ~XmlBuilder();

  void Reset();

  // Take ownership of the built element; second call returns NULL
  XmlElement * CreateElement();

  // Peek at the built element without taking ownership
  XmlElement * BuiltElement();

private:
  XmlElement * pelCurrent_;
  scoped_ptr<XmlElement> pelRoot_;
  scoped_ptr<std::vector<XmlElement *, std::allocator<XmlElement *> > >
    pvParents_;
};

}

#endif
