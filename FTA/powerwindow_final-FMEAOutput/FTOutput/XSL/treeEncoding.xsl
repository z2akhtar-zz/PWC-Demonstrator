<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
  <xsl:param name="IndividualIDParam"/>

  <xsl:template match="/">
    <xsl:apply-templates select="HiP-HOPS_Results/CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/NSGA"></xsl:apply-templates>


  </xsl:template>
  
  <xsl:template match="HiP-HOPS_Results/NSGA">
   <xsl:apply-templates select="ArchiveNonDominatedPopulations/NonDominatedPopulation"/>

  </xsl:template>
<xsl:template match="NonDominatedPopulation">
  <TABLE id="TreeViewTable" cellspacing="0">
    <TR>
      <TD>
        <xsl:element name="SPAN">
          <xsl:attribute name="class">TreeviewSpanArea</xsl:attribute>
          <xsl:attribute name="id">TreeviewSpanZone</xsl:attribute>
          <xsl:element name="img">
            <xsl:attribute name="src">FTOutput/images/circle.gif</xsl:attribute>
            <xsl:attribute name="onload">
            USETEXTLINKS = 1;
            STARTALLOPEN = 0;
            HIGHLIGHT = 1;
            PRESERVESTATE = 1;
            GLOBALTARGET="R";
            ICONPATH = 'FTOutput/images/';
            foldersTree = gFld("Model","javascript:toggleAllFolders(foldersTree)");

            <xsl:choose>
              <xsl:when test="$IndividualIDParam=''">
                <xsl:variable name="auxVal" select="'foldersTree'" />
                <xsl:apply-templates select="Individual/Encoding/TreeEncoding/EncodingNode"/>

                <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual/Encoding/TreeEncoding/EncodingNode">
                  <xsl:variable name="subAuxVal" select="generate-id()" />
                  <xsl:if test="position() &gt; 1"> , </xsl:if>
                  aux<xsl:value-of select="$subAuxVal" />
                </xsl:for-each>

                ]);
              </xsl:when>
              <xsl:otherwise>
                <xsl:variable name="auxVal" select="'foldersTree'" />
                <xsl:apply-templates select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeEncoding/EncodingNode"/>

                <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeEncoding/EncodingNode">
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

  <xsl:template match="HiP-HOPS_Results/CoitGA">
    <xsl:apply-templates select="Population"/>

  </xsl:template>
  <xsl:template match="Population">
    <TABLE id="TreeViewTable" cellspacing="0">
      <TR>
        <TD>
          <xsl:element name="SPAN">
            <xsl:attribute name="class">TreeviewSpanArea</xsl:attribute>
            <xsl:attribute name="id">TreeviewSpanZone</xsl:attribute>
            <xsl:element name="img">
              <xsl:attribute name="src">FTOutput/images/circle.gif</xsl:attribute>
              <xsl:attribute name="onload">
              USETEXTLINKS = 1;
              STARTALLOPEN = 0;
              HIGHLIGHT = 1;
              PRESERVESTATE = 1;
              GLOBALTARGET="R";
              ICONPATH = 'FTOutput/images/';
              foldersTree = gFld("Model","javascript:toggleAllFolders(foldersTree)");

              <xsl:choose>
                <xsl:when test="$IndividualIDParam=''">
                  <xsl:variable name="auxVal" select="'foldersTree'" />
                  <xsl:apply-templates select="Individual/Encoding/TreeEncoding/EncodingNode"/>

                  <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual/Encoding/TreeEncoding/EncodingNode">
                    <xsl:variable name="subAuxVal" select="generate-id()" />
                    <xsl:if test="position() &gt; 1"> , </xsl:if>
                    aux<xsl:value-of select="$subAuxVal" />
                  </xsl:for-each>

                  ]);
                </xsl:when>
                <xsl:otherwise>
                  <xsl:variable name="auxVal" select="'foldersTree'" />
                  <xsl:apply-templates select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeEncoding/EncodingNode"/>

                  <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeEncoding/EncodingNode">
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

  <xsl:template match="EncodingNode">
    <xsl:variable name="auxVal" select="generate-id()" />
    
    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="Component/Name"/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "Component.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "Component.gif";

    <xsl:apply-templates select="Component/Implementation"/>
    <xsl:apply-templates select="Component/Allocation" />

    aux<xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Component/Implementation">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>
    <xsl:for-each select="Component/Allocation">
      <xsl:variable name="subAuxVal2" select="generate-id()" />
      , aux<xsl:value-of select="$subAuxVal2" />
    </xsl:for-each>
    ]);

  </xsl:template>

  <xsl:template match="Implementation">
    <xsl:variable name="auxVal" select="generate-id()" />
    <xsl:variable name="auxValStr">
      aux<xsl:value-of select="$auxVal" />
    </xsl:variable>

    <xsl:value-of select="$auxValStr" />  = gFld("<xsl:value-of select="Name"/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    <xsl:value-of select="$auxValStr" />.xID = "f<xsl:value-of select="$auxVal" />";
    <xsl:value-of select="$auxValStr" />.iconSrc = ICONPATH + "Implementation.gif";
    <xsl:value-of select="$auxValStr" />.iconSrcClosed = ICONPATH + "Implementation.gif";

   <xsl:apply-templates select="../../SubNodes">
    <xsl:with-param name="parentVal" select="$auxValStr"/>
  </xsl:apply-templates>




</xsl:template>
  <xsl:template match="SubNodes" name="SubNodes">
    <xsl:param name="parentVal"/>
    <xsl:apply-templates/>
   
    <xsl:value-of select="$parentVal" />.addChildren([<xsl:for-each select="*">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>

    ]);

  </xsl:template>
  <xsl:template name="Allocation">
    <xsl:variable name="auxVal" select="generate-id()" />
    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="."/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "allocation.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "allocation.gif";


  </xsl:template>
 
</xsl:stylesheet>