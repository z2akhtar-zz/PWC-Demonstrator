<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:param name="XmlFileName"/>
  <xsl:param name="Order"/>
  <xsl:param name="FMEAType"/>
  <xsl:param name="PageSize" />
  <xsl:param name="Page" />
  <xsl:template match="HiP-HOPS_Results">

    <xsl:variable name ="numRows">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(FaultTrees/FMEA/Component/Events/*/Effects/Effect[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(FaultTrees/FMEA/Component/Events/*/Effects/Effect[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(FaultTrees/FMEA/Component/Events/*/Effects[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(FaultTrees/FMEA/Component/Events/*/Effects/Effect)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>


    <xsl:variable name="numPages" select="ceiling(($numRows - 1) div $PageSize)" />
    <table>
      <tr>
        <td>
          Show FMEA results of:
          <xsl:element name='select'>
            <xsl:attribute name='id'>FMEATypeSelect</xsl:attribute>
            <xsl:attribute name='size'>1</xsl:attribute>
            <xsl:attribute name='onchange'>

              javascript:
              fmeaTransform('<xsl:value-of select="$XmlFileName" />',
              document.getElementById('FMEATypeSelect').options[document.getElementById('FMEATypeSelect').selectedIndex].value,
              '0',
              '100');
            </xsl:attribute>
            <xsl:element name='option'>
              <xsl:attribute name='value'>DirectEffects</xsl:attribute>
              <xsl:if test='$FMEAType = "DirectEffects"'>
                <xsl:attribute name='selected' />
              </xsl:if>
              Direct Effects
            </xsl:element>
            <xsl:element name='option'>
              <xsl:attribute name='value'>FurtherEffects</xsl:attribute>
              <xsl:if test='$FMEAType = "FurtherEffects"'>
                <xsl:attribute name='selected' />
              </xsl:if>
              Further Effects
            </xsl:element>
            <xsl:element name='option'>
              <xsl:attribute name='value'>DirectAndFurtherEffects</xsl:attribute>
              <xsl:if test='$FMEAType = "DirectAndFurtherEffects"'>
                <xsl:attribute name='selected' />
              </xsl:if>
              Direct and Further Effects
            </xsl:element>
            <xsl:element name='option'>
              <xsl:attribute name='value'>NoEffects</xsl:attribute>
              <xsl:if test='$FMEAType = "NoEffects"'>
                <xsl:attribute name='selected' />
              </xsl:if>
              No Effects
            </xsl:element>


          </xsl:element>


        </td>
        <td>
          Number of rows per page:
          <xsl:element name='select'>
            <xsl:attribute name='id'>PageSizeSelect</xsl:attribute>
            <xsl:attribute name='size'>1</xsl:attribute>
            <xsl:attribute name='onchange'>
              javascript:

              var newPageSize = document.getElementById('PageSizeSelect').options[document.getElementById('PageSizeSelect').selectedIndex].value;
              var newNumPages = Math.ceil(<xsl:value-of select="$numRows - 1"/> / newPageSize);
              var newPage = <xsl:value-of select="$Page"/> ;
              if(newPage + 1 &gt; newNumPages)
              {
              newPage = newNumPages - 1;
              }

              fmeaTransform('<xsl:value-of select="$XmlFileName" />',
              '<xsl:value-of select="$FMEAType"/>',
              newPage,
              newPageSize);


            </xsl:attribute>
            <xsl:element name='option'>
              <xsl:attribute name='value'>100</xsl:attribute>
              <xsl:if test='$PageSize = 100'>
                <xsl:attribute name='selected' />
              </xsl:if>
              100
            </xsl:element>
            <xsl:element name='option'>
              <xsl:attribute name='value'>200</xsl:attribute>
              <xsl:if test='$PageSize = 200'>
                <xsl:attribute name='selected' />
              </xsl:if>
              200
            </xsl:element>
            <xsl:element name='option'>
              <xsl:attribute name='value'>400</xsl:attribute>
              <xsl:if test='$PageSize = 400'>
                <xsl:attribute name='selected' />
              </xsl:if>
              400
            </xsl:element>
            <xsl:element name='option'>
              <xsl:if test='($PageSize != 100) and ($PageSize != 200) and ($PageSize != 400)'>
                <xsl:attribute name='selected' />
              </xsl:if>
              <xsl:attribute name='value'>
                <xsl:value-of select='$numRows'/>
              </xsl:attribute>
              All
            </xsl:element>

          </xsl:element>

        </td>
      </tr>
    </table>
    <div id="header">
      <ul id="primary">
        <li>
          <span>
            FMEA
          </span>
          <ul id="secondary">
            <xsl:choose>

              <xsl:when test="$Page &gt; 0">

                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      fmeaTransform('<xsl:value-of select="$XmlFileName" />',
                      '<xsl:value-of select="$FMEAType"/>',
                      '0',
                      '<xsl:value-of select="$PageSize"/>');

                    </xsl:attribute>
                    First Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>First Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:choose>

              <xsl:when test="$Page &gt; 0">

                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      fmeaTransform('<xsl:value-of select="$XmlFileName" />',
                      '<xsl:value-of select="$FMEAType"/>',
                      '<xsl:value-of select="$Page - 1"/>',
                      '<xsl:value-of select="$PageSize"/>');
                    </xsl:attribute>
                    Previous Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>Previous Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <li>
              <span>
                Current Page: <xsl:value-of select="$Page + 1"/> of <xsl:value-of select="$numPages"/>
              </span>
            </li>
            <xsl:choose>
              <xsl:when test="$Page + 1 &lt; $numPages">
                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      fmeaTransform('<xsl:value-of select="$XmlFileName" />',
                      '<xsl:value-of select="$FMEAType"/>',
                      '<xsl:value-of select="$Page + 1"/>',
                      '<xsl:value-of select="$PageSize"/>');
                    </xsl:attribute>
                    Next Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>Next Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
            <xsl:choose>
              <xsl:when test="$Page + 1 &lt; $numPages">
                <li>
                  <xsl:element name="a">
                    <xsl:attribute name="href">
                      javascript:
                      fmeaTransform('<xsl:value-of select="$XmlFileName" />',
                      '<xsl:value-of select="$FMEAType"/>',
                      '<xsl:value-of select="$numPages - 1"/>',
                      '<xsl:value-of select="$PageSize"/>');
                    </xsl:attribute>
                    Last Page
                  </xsl:element>
                </li>
              </xsl:when>
              <xsl:otherwise>
                <li>
                  <span>Last Page</span>
                </li>
              </xsl:otherwise>
            </xsl:choose>
          </ul>
        </li>
      </ul>
    </div>
    <xsl:apply-templates select="FaultTrees/FMEA" />
  </xsl:template>

  <xsl:template match="FMEA">
    <div id="main">
      <TABLE id="ContentTable" cellspacing="0">
        <xsl:if test="count(Component)>0">
          <TR>
            <TD>
              <TABLE id="CutSetTable" cellSpacing="0">

                <xsl:apply-templates select="Component" />
              </TABLE>
            </TD>
          </TR>
        </xsl:if>
      </TABLE>
    </div>
  </xsl:template>

  <!-- Component template -->

  <xsl:template match="Component">

    <xsl:variable name ="NumEffects">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(Events/*/Effects/Effect[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(Events/*/Effects/Effect[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(Events/*/Effects[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(Events/*/Effects/Effect)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name ="NumPrecedingEffects">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Events/*/Effects/Effect[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Events/*/Effects/Effect[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Events/*/Effects[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(preceding-sibling::*/Events/*/Effects/Effect)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>



    <xsl:if test="$NumPrecedingEffects + $NumEffects &gt;= ($Page * $PageSize)">
      <xsl:if test="$NumPrecedingEffects &lt; $PageSize + ($PageSize * $Page)">
        <xsl:apply-templates select="Events/*">
          <xsl:with-param name ="NumComponentEffects" select="$NumEffects" />
          <xsl:with-param name ="ComponentPosition" select="$NumPrecedingEffects" />
        </xsl:apply-templates>

      </xsl:if>
    </xsl:if>

  </xsl:template>

  <!-- General Event template -->
  <xsl:template name="GeneralEvent">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:variable name ="NumEventEffects">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(Effects/Effect[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(Effects/Effect[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(Effects[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(Effects/Effect)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="EventImageSrc">FTOutput/images/failureMode.gif</xsl:variable>

    <xsl:variable name ="EventPosition">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Effects/Effect[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Effects/Effect[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(preceding-sibling::*/Effects[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(preceding-sibling::*/Effects/Effect)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>


    <xsl:variable name="NumPrecedingEffects">
      <xsl:value-of select="$EventPosition + $ComponentPosition"/>
    </xsl:variable>


    <xsl:if test="$NumPrecedingEffects + $NumEventEffects &gt;= ($Page * $PageSize)">
      <xsl:if test="$NumPrecedingEffects &lt; $PageSize + ($PageSize * $Page)">
        <xsl:choose>

          <xsl:when test="$FMEAType = 'DirectEffects'">
            <xsl:apply-templates select="Effects/Effect[./SinglePointFailure='true']">
              <xsl:with-param name ="EventImageSrc" select="$EventImageSrc" />
              <xsl:with-param name ="NumPrecedingEffects" select="$NumPrecedingEffects" />
              <xsl:with-param name ="ComponentPosition" select="$ComponentPosition" />
              <xsl:with-param name ="EventPosition" select="$EventPosition" />
              <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />
              <xsl:with-param name ="NumEventEffects" select="$NumEventEffects" />
            </xsl:apply-templates>

          </xsl:when>
          <xsl:when test="$FMEAType = 'FurtherEffects'">
            <xsl:apply-templates select="Effects/Effect[./SinglePointFailure='false']">
              <xsl:with-param name ="EventImageSrc" select="$EventImageSrc" />
              <xsl:with-param name ="NumPrecedingEffects" select="$NumPrecedingEffects" />
              <xsl:with-param name ="ComponentPosition" select="$ComponentPosition" />
              <xsl:with-param name ="EventPosition" select="$EventPosition" />
              <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />
              <xsl:with-param name ="NumEventEffects" select="$NumEventEffects" />
            </xsl:apply-templates>

          </xsl:when>
          <xsl:when test="$FMEAType = 'NoEffects'">
            <xsl:if test="$NumEventEffects &gt; 0">
              <xsl:call-template name="GenerateNoEffectColumns">
                <xsl:with-param name="EffectPosition" select="0"/>
                <xsl:with-param name="EventImageSrc" select="$EventImageSrc"/>
                <xsl:with-param name="NumPrecedingEffects" select="0"/>
                <xsl:with-param name ="ComponentPosition" select="$ComponentPosition" />
                <xsl:with-param name ="EventPosition" select="$EventPosition" />
                <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />
                <xsl:with-param name ="NumEventEffects" select="$NumEventEffects" />
              </xsl:call-template>
            </xsl:if>
          </xsl:when>
          <xsl:otherwise>
            <xsl:apply-templates select="Effects/Effect">
              <xsl:with-param name ="EventImageSrc" select="$EventImageSrc" />
              <xsl:with-param name ="NumPrecedingEffects" select="$NumPrecedingEffects" />
              <xsl:with-param name ="ComponentPosition" select="$ComponentPosition" />
              <xsl:with-param name ="EventPosition" select="$EventPosition" />
              <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />
              <xsl:with-param name ="NumEventEffects" select="$NumEventEffects" />
            </xsl:apply-templates>

          </xsl:otherwise>
        </xsl:choose>

      </xsl:if>
    </xsl:if>


  </xsl:template>

  <!-- Basic Event template -->

  <xsl:template match="BasicEvent">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>


  </xsl:template>
  
  
  <!-- Circle template -->

  <xsl:template match="Circle">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>


  </xsl:template>

  <!-- Normal Event template -->

  <xsl:template match="NormalEvent">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>
  </xsl:template>
  <!-- Input Deviation template -->

  <xsl:template match="InputDeviationNode">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>
  </xsl:template>
  <!-- Output Deviation template -->

  <xsl:template match="OutputDeviationNode">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>
  </xsl:template>
  
  <!-- Exported Deviation template -->

  <xsl:template match="ExportedDeviationNode">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>
  </xsl:template>
  
  <!-- PCCF template -->

  <xsl:template match="PCCFNode">
    <xsl:param name="ComponentPosition" />
    <xsl:param name="NumComponentEffects" />

    <xsl:call-template name="GeneralEvent">
      <xsl:with-param name="ComponentPosition" select="$ComponentPosition"/>
      <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />

    </xsl:call-template>
  </xsl:template>
  
  <!-- Effect template -->

  <xsl:template match="Effect">
    <xsl:param name="EventImageSrc" />
    <xsl:param name="NumPrecedingEffects" />
    <xsl:param name="ComponentPosition" />
    <xsl:param name="EventPosition" />
    <xsl:param name="NumComponentEffects" />
    <xsl:param name="NumEventEffects" />

    <xsl:variable name ="EffectPosition">
      <xsl:choose>
        <xsl:when test="$FMEAType = 'DirectEffects'">
          <xsl:value-of select="count(preceding-sibling::*[./SinglePointFailure='true'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'FurtherEffects'">
          <xsl:value-of select="count(preceding-sibling::*[./SinglePointFailure='false'])"/>
        </xsl:when>
        <xsl:when test="$FMEAType = 'NoEffects'">
          <xsl:value-of select="count(preceding-sibling::*[not(Effect)])"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="count(preceding-sibling::*)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>

    <xsl:variable name="NewNumPrecedingEffects" select="$NumPrecedingEffects + $EffectPosition" />

    <xsl:if test="$NumPrecedingEffects + $NumEventEffects &gt;= ($Page * $PageSize)">
      <xsl:if test="$NumPrecedingEffects &lt; $PageSize + ($PageSize * $Page)">

        <xsl:call-template name="GenerateColumns">
          <xsl:with-param name="CutSetPosition" select="0"/>
          <xsl:with-param name="CutSetsPosition" select="0"/>
          <xsl:with-param name="EffectPosition" select="$EffectPosition"/>
          <xsl:with-param name="EventImageSrc" select="$EventImageSrc"/>
          <xsl:with-param name="NumPrecedingEffects" select="$NewNumPrecedingEffects"/>
          <xsl:with-param name ="ComponentPosition" select="$ComponentPosition" />
          <xsl:with-param name ="EventPosition" select="$EventPosition" />
          <xsl:with-param name ="NumComponentEffects" select="$NumComponentEffects" />
          <xsl:with-param name ="NumEventEffects" select="$NumEventEffects" />
        </xsl:call-template>

      </xsl:if>
    </xsl:if>
  </xsl:template>


  <!-- Generate Columns template -->

  <xsl:template name="GenerateColumns">
    <xsl:param name="EventImageSrc" />
    <xsl:param name="EffectPosition" />
    <xsl:param name="NumPrecedingEffects" />
    <xsl:param name="ComponentPosition" />
    <xsl:param name="EventPosition" />
    <xsl:param name="NumComponentEffects" />
    <xsl:param name="NumEventEffects" />

    <xsl:if test="$NumPrecedingEffects &gt;= ($Page * $PageSize)">
      <xsl:if test="$NumPrecedingEffects &lt; $PageSize + ($PageSize * $Page)">

        <xsl:variable name ="EffectName" select="Name"/>
        <xsl:variable name ="ComponentName" select="../../../../Name"/>

        <xsl:if test="($EventPosition=0 and $EffectPosition=0) or $NumPrecedingEffects = ($Page * $PageSize)">
          <xsl:if test="$NumComponentEffects &gt; 0">
            <TR class="ComponentHeader">
              <xsl:element name="TD">
                <xsl:attribute name="colspan">
                  4
                </xsl:attribute>
                Component: <xsl:value-of select="$ComponentName" />
              </xsl:element>
            </TR>
            <TR class="HeaderRow">
              <TD>Failure Mode</TD>
              <TD>System Effect</TD>
              <TD>Severity</TD>
              <TD>Single Point of Failure</TD>
            </TR>
          </xsl:if>
        </xsl:if>


        <xsl:element name="TR">
          <xsl:attribute name="class">DataRow</xsl:attribute>

          <xsl:if test="($EffectPosition=0) or $NumPrecedingEffects = ($Page * $PageSize)">
            <xsl:element name="TD">

              <xsl:attribute name="rowspan">
                <xsl:value-of select="$NumEventEffects - ($NumPrecedingEffects - $EventPosition - $ComponentPosition)" />

              </xsl:attribute>
              <Table id="EventsTable">
                <Tr>
                  <Td>
                    <DIV class="FailureModesCell">
                      <xsl:element name="IMG">
                        <xsl:attribute name="src">
                          <xsl:value-of select="$EventImageSrc"/>
                        </xsl:attribute>
                      </xsl:element>
                      <xsl:value-of select="../../ShortName"/> (<xsl:value-of select="../../@ID"/>)
                    </DIV>
                  </Td>
                </Tr>

              </Table>


            </xsl:element>
          </xsl:if>
          <xsl:element name="TD">


            <xsl:attribute name="class">
              EffectsCell
            </xsl:attribute>
            <xsl:element name="a">
              <xsl:attribute name="href">
                javascript:
                topEventTransform('FaultTrees.xml','<xsl:value-of select="Name" />');
                cutSetsTransform('CutSets(<xsl:value-of select="@ID" />).xml','<xsl:value-of select="Name" />', 'unavailability', 'ascending','100', '0', '0','<xsl:value-of select="../../@ID" />');
                menuTransform('FaultTrees.xml','Cut Sets View: <xsl:value-of select="Name" />');


              </xsl:attribute>
              <xsl:value-of select="$EffectName"/>
            </xsl:element>


          </xsl:element>

          <xsl:element name="TD">

            <xsl:attribute name="class">
              EffectsCell
            </xsl:attribute>

            <xsl:value-of select="/HiP-HOPS_Results/FaultTrees/FaultTree[./Name=$EffectName]/Severity" />


          </xsl:element>


          <TD>



            <xsl:value-of select="SinglePointFailure" />

          </TD>

        </xsl:element>
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <!-- Generate No Effect Columns template -->

  <xsl:template name="GenerateNoEffectColumns">
    <xsl:param name="EventImageSrc" />
    <xsl:param name="EffectPosition" />
    <xsl:param name="NumPrecedingEffects" />
    <xsl:param name="ComponentPosition" />
    <xsl:param name="EventPosition" />
    <xsl:param name="NumComponentEffects" />
    <xsl:param name="NumEventEffects" />

    <xsl:if test="$NumPrecedingEffects &gt;= ($Page * $PageSize)">
      <xsl:if test="$NumPrecedingEffects &lt; $PageSize + ($PageSize * $Page)">

        <xsl:variable name ="ComponentName" select="../../Name"/>

        <xsl:if test="($EventPosition=0 and $EffectPosition=0) or $NumPrecedingEffects = ($Page * $PageSize)">
          <xsl:if test="$NumComponentEffects &gt; 0">
            <TR class="ComponentHeader">
              <xsl:element name="TD">
                <xsl:attribute name="colspan">
                  2
                </xsl:attribute>
                Component: <xsl:value-of select="$ComponentName" />
              </xsl:element>
            </TR>
            <TR class="HeaderRow">
              <TD>Failure Mode</TD>
              <TD>System Effect</TD>
            </TR>
          </xsl:if>
        </xsl:if>


        <xsl:element name="TR">
          <xsl:attribute name="class">DataRow</xsl:attribute>

          <xsl:if test="($EffectPosition=0) or $NumPrecedingEffects = ($Page * $PageSize)">
            <xsl:element name="TD">


              <Table id="EventsTable">
                <Tr>
                  <Td>
                    <DIV class="FailureModesCell">
                      <xsl:element name="IMG">
                        <xsl:attribute name="src">
                          <xsl:value-of select="$EventImageSrc"/>
                        </xsl:attribute>
                      </xsl:element>
                      <xsl:value-of select="ShortName"/> (<xsl:value-of select="@ID"/>)
                    </DIV>
                  </Td>
                </Tr>

              </Table>


            </xsl:element>
          </xsl:if>

          <TD>



            none

          </TD>

        </xsl:element>
      </xsl:if>
    </xsl:if>
  </xsl:template>

  <xsl:template name ="max">
    <xsl:param name ="list" />
    <xsl:choose >
      <xsl:when test ="$list">
        <xsl:variable name ="first" select ="$list[1]" />

        <xsl:variable name ="rest">
          <xsl:call-template name ="max">
            <xsl:with-param name ="list" select ="$list[position() != 1]" />
          </xsl:call-template>
        </xsl:variable>

        <xsl:choose>
          <xsl:when test="$first &gt; $rest">
            <xsl:value-of select ="$first"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select ="$rest"/>
          </xsl:otherwise>
        </xsl:choose>

      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>

    </xsl:choose>

  </xsl:template>

  <xsl:template name="generateTableHeader">
    <xsl:param name="num">1</xsl:param>
    <xsl:param name="maxOrder" />
    <!-- param has initial value of 1 -->
    <xsl:if test="not($num = $maxOrder +1)">
      <xsl:if test="FaultTrees/FaultTree/AllCutSets/CutSets[@order = $num]" >
        <xsl:element name='option'>
          <xsl:attribute name='value'>
            <xsl:value-of select="$num" />
          </xsl:attribute>
          <xsl:if test='$Order = $num'>
            <xsl:attribute name='selected' />
          </xsl:if>
          <xsl:value-of select="$num" />
        </xsl:element>
      </xsl:if>
      <xsl:call-template name="generateTableHeader">
        <xsl:with-param name="num">
          <xsl:value-of select="$num + 1" />

        </xsl:with-param>
        <xsl:with-param name="maxOrder">
          <xsl:value-of select="$maxOrder" />

        </xsl:with-param>
      </xsl:call-template>
    </xsl:if>
  </xsl:template>
</xsl:stylesheet>

