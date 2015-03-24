<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"

>
  <xsl:param name="IndividualIDParam"/>

  <xsl:template match="/">
    <xsl:apply-templates select="HiP-HOPS_Results/CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/NSGA"></xsl:apply-templates>

    <xsl:apply-templates select="HiP-HOPS_Results/SafetyAllocations"></xsl:apply-templates>

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
                <xsl:apply-templates select="Individual/Encoding/TreeASILEncoding/EncodingNode"/>

                <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual/Encoding/TreeASILEncoding/EncodingNode">
                  <xsl:variable name="subAuxVal" select="generate-id()" />
                  <xsl:if test="position() &gt; 1"> , </xsl:if>
                  aux<xsl:value-of select="$subAuxVal" />
                </xsl:for-each>

                ]);
              </xsl:when>
              <xsl:otherwise>
                <xsl:variable name="auxVal" select="'foldersTree'" />
                <xsl:apply-templates select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeASILEncoding/EncodingNode"/>

                <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeASILEncoding/EncodingNode">
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

  <xsl:template match="HiP-HOPS_Results/SafetyAllocations">
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
                  <xsl:apply-templates select="Individual/Encoding/TreeASILEncoding/EncodingNode"/>

                  <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual/Encoding/TreeASILEncoding/EncodingNode">
                    <xsl:variable name="subAuxVal" select="generate-id()" />
                    <xsl:if test="position() &gt; 1"> , </xsl:if>
                    aux<xsl:value-of select="$subAuxVal" />
                  </xsl:for-each>

                  ]);
                </xsl:when>
                <xsl:otherwise>
                  <xsl:variable name="auxVal" select="'foldersTree'" />
                  <xsl:apply-templates select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeASILEncoding/EncodingNode"/>

                  <xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Individual[./IndividualID=$IndividualIDParam]/Encoding/TreeASILEncoding/EncodingNode">
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
    
    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="Component/ShortName"/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "Component.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "Component.gif";


    <xsl:apply-templates select="Component/BasicEvents/BasicEvent"/>
    <xsl:apply-templates select="SubNodes/EncodingNode"/>

    aux<xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="Component/BasicEvents/BasicEvent">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>
    <xsl:for-each select="SubNodes/EncodingNode">
      <xsl:variable name="subAuxVal2" select="generate-id()" />
      <xsl:if test="(position() &gt; 1) or (../../Component/BasicEvents/BasicEvent)"> , </xsl:if> aux<xsl:value-of select="$subAuxVal2" />
    </xsl:for-each>
    ]);

  </xsl:template>

 
  <xsl:template match="BasicEvent">
    <xsl:variable name="auxVal" select="generate-id()" />
    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="Name"/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "failureMode.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "failureMode.gif";

<xsl:apply-templates select="SafetyRequirement"/>

    aux<xsl:value-of select="$auxVal" />.addChildren([<xsl:for-each select="SafetyRequirement">
      <xsl:variable name="subAuxVal" select="generate-id()" />
      <xsl:if test="position() &gt; 1"> , </xsl:if>
      aux<xsl:value-of select="$subAuxVal" />
    </xsl:for-each>
    
    ]);

  </xsl:template>

  <xsl:template match="SafetyRequirement">
    <xsl:variable name="auxVal" select="generate-id()" />
    aux<xsl:value-of select="$auxVal" />  = gFld("<xsl:value-of select="."/>", "javascript:toggleAllFolders(aux<xsl:value-of select="$auxVal" />)");
    aux<xsl:value-of select="$auxVal" />.xID = "f<xsl:value-of select="$auxVal" />";
    aux<xsl:value-of select="$auxVal" />.iconSrc = ICONPATH + "sil.gif";
    aux<xsl:value-of select="$auxVal" />.iconSrcClosed = ICONPATH + "sil.gif";

  </xsl:template>
 
</xsl:stylesheet>