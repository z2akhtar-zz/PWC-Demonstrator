<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hip-hops.eu/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
  <xsl:param name="XmlFileName"/>
  <xsl:param name="ftName"/>
  <xsl:template match="HiP-HOPS_Results">
    <xsl:apply-templates select="FaultTrees" />
  </xsl:template>
  <xsl:template match="FaultTrees">
    <div id="header">
      <ul id="primary">
        <li>
          <span>Fault Trees</span>
        </li>
        <li>
          <xsl:element name="a">
            <xsl:attribute name="href">
              javascript:
              topEventTransform('Cutsets(<xsl:value-of select="FaultTree[./Name=$ftName]/@ID" />).xml','<xsl:value-of select="$ftName" />');
              cutSetsTransform('Cutsets(<xsl:value-of select="FaultTree[./Name=$ftName]/@ID" />).xml','<xsl:value-of select="$ftName" />', 'unavailability', 'ascending','100', '0', '0', '0');
              menuTransform('FaultTrees.xml','Cut Sets View: <xsl:value-of select="$ftName" />');

            </xsl:attribute>
            Cut Sets
          </xsl:element>
        </li>

      </ul>
    </div>
      <TABLE id="TreeViewTable" cellspacing="0">
          <TR>
            <TD>
              <xsl:element name="SPAN">
                <xsl:attribute name="class">TreeviewSpanArea</xsl:attribute>
                <xsl:attribute name="id">TreeviewSpanZone</xsl:attribute>
                <xsl:element name="img">
                  <xsl:attribute name="src">FTOutput/images/circle.gif</xsl:attribute>
				          <xsl:attribute name="onload">
                  USETEXTLINKS = 1
                  STARTALLOPEN = 0
                  HIGHLIGHT = 1
                  PRESERVESTATE = 1
                  GLOBALTARGET="R"
                  ICONPATH = 'FTOutput/images/'
                  foldersTree = gFld("FaultTrees","javascript:toggleAllFolders(foldersTree)")
              
              <xsl:choose>
                  <xsl:when test="$ftName=''">
                    <xsl:variable name="auxVal" select="'foldersTree'" />
                    <xsl:apply-templates select="FaultTree"/>

                    <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="FaultTree">
                      <xsl:variable name="subAuxVal" select="generate-id()" />
                      <xsl:if test="position() &gt; 1"> , </xsl:if>
                      aux<xsl:value-of select="$subAuxVal" />
                    </xsl:for-each>

                    ]);
                  </xsl:when>
                <xsl:otherwise>
                  <xsl:variable name="auxVal" select="'foldersTree'" />
                  <xsl:apply-templates select="FaultTree[./Name=$ftName]"/>

                  <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="FaultTree[./Name=$ftName]">
                    <xsl:variable name="subAuxVal" select="generate-id()" />
                    <xsl:if test="position() &gt; 1"> , </xsl:if>
                    aux<xsl:value-of select="$subAuxVal" />
                  </xsl:for-each>

                  ]);

                </xsl:otherwise>

                  </xsl:choose>




                    foldersTree.treeID = "L1"
                    foldersTree.xID = "bigtree"
                    foldersTree.iconSrc = ICONPATH + "model.gif";
                    foldersTree.iconSrcClosed = ICONPATH + "model.gif";
                    initializeDocument();
                    this.parentNode.removeChild(this);
                  </xsl:attribute>
                </xsl:element>


                <xsl:element name="NOSCRIPT">
                  A tree for site navigation will open here if you enable JavaScript in your browser.
                </xsl:element>
              </xsl:element>
                         
            </TD>
          </TR>
        </TABLE>      
  
  </xsl:template>


  <xsl:template match="FaultTree">
    <xsl:variable name="auxVal" select="generate-id()" />

    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="Name"/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "FaultTree.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "FaultTree.gif";

    <xsl:apply-templates select="OutputDeviation"/>

    aux<xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="OutputDeviation">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>

    ]);

  </xsl:template>

  <xsl:template match="OutputDeviation">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "OutputDeviation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "OutputDeviation.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>
  
  <xsl:template match="InputDeviation">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "InputDeviation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "InputDeviation.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>
  <xsl:template match="NullOp">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "nullop.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "nullop.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>
  
  <xsl:template match="Not">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "not.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "not.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>

  <xsl:template match="Children" name="Children">
    <xsl:param name="parentVal"/>
    <xsl:apply-templates/>

    <xsl:value-of select="$parentVal" />.addChildren([<xsl:for-each select="*">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>

    ]);

  </xsl:template>


  <xsl:template match="Or">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "Or.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "Or.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>

  <xsl:template match="And">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "and.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "and.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>



  </xsl:template>

  <xsl:template match="Transfer">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("Transfer to (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "transfer.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "transfer.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
    </xsl:apply-templates>


  </xsl:template>

  <xsl:template match="BasicEvent">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "failuremode.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "failuremode.gif";
  </xsl:template>
  
  <xsl:template match="InputDeviationNode">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "inputdeviation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "inputdeviation.gif";
  </xsl:template>
  
  <xsl:template match="OutputDeviationNode">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "outputdeviation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "outputdeviation.gif";
  </xsl:template>
  
  <xsl:template match="ExportedDeviationNode">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "outputdeviation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "outputdeviation.gif";
  </xsl:template>
  
  <xsl:template match="PCCFNode">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "proxy.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "proxy.gif";
  </xsl:template>
  
  <xsl:template match="NormalEvent">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "normalEvent.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "normalEvent.gif";



  </xsl:template>
  
  <xsl:template match="Circle">
    <xsl:param name="eventAuxVal"/>
    <xsl:variable name="auxVal" select="$eventAuxVal" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "circle.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "circle.gif";

  </xsl:template>

  <xsl:template match="Goto">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "proxy.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "proxy.gif";

    <xsl:apply-templates select="Children">
      <xsl:with-param name="parentVal" select="$auxValStr"/>
  </xsl:apply-templates>

  </xsl:template>

  <xsl:template match="PCCF">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/> (<xsl:value-of select="@ID"/>)", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "proxy.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "proxy.gif";

    <xsl:apply-templates select="Children">
    <xsl:with-param name="parentVal" select="$auxValStr"/>
  </xsl:apply-templates>



  </xsl:template>

  <xsl:template match="Event">
    <xsl:variable name="eventID" select="@ID"/>
    <xsl:apply-templates select="/HiP-HOPS_Results/FaultTrees/FMEA/Component/Events/*[@ID=$eventID]">
      <xsl:with-param name="eventAuxVal" select="generate-id()"/>
    </xsl:apply-templates>
    
  </xsl:template>


  
  </xsl:stylesheet>