<?xml version="1.0"?>
<xsl:stylesheet version="1.0"
      xmlns:faultTree ="http://www.hiphops.com/namespace/faultTree"
	  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:param name="XmlFileName"/>
  <xsl:param name="sortType"/>
  <xsl:param name="sortOrder"/>
  <xsl:template match="/">
    <xsl:apply-templates select="HiP-HOPS_Results/CoitGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/NSGA"></xsl:apply-templates>
    <xsl:apply-templates select="HiP-HOPS_Results/SafetyAllocations"></xsl:apply-templates>
    
  </xsl:template>
  
<xsl:template match="HiP-HOPS_Results/NSGA">
  <TABLE id="ContentTable" cellSpacing="0">
			<TR>
			<TD>
        <xsl:choose>
					 <xsl:when test="ArchiveNonDominatedPopulations">
						<TABLE id="CutSetTable" cellSpacing="0">
							<TR class="HeaderRow">
								<xsl:for-each select="Objectives/Objective">
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                       javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);

                      </xsl:attribute>
                      <xsl:value-of select="Name" />
                    </xsl:element>
									</TD>
								</xsl:for-each>	
								<TD>
									Configuration
								</TD>
							</TR>
							<xsl:for-each select="ArchiveNonDominatedPopulations/NonDominatedPopulation/Individual">
                <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
								<TR class="DataRow">
									<xsl:for-each select="Evaluations/Evaluation">
										<TD>
										<xsl:value-of select="Value" />
									</TD>
									</xsl:for-each>
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                                  
javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');





                      </xsl:attribute>
                      Click here to see configuration
                    </xsl:element>
									</TD>
								</TR>
							</xsl:for-each>	
						</TABLE>
					 </xsl:when>
					 <xsl:otherwise>
						<TABLE id="CutSetTable" cellSpacing="0">
							<TR class="HeaderRow">
								<xsl:for-each select="Objectives/*">
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);

                      </xsl:attribute>
                      <xsl:value-of select="Name" />
                    </xsl:element>
									</TD>
								</xsl:for-each>	
								<TD>
									Configuration
								</TD>
							</TR>
							<xsl:for-each select="NonDominatedPopulation/Individual">
                <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
								<TR class="DataRow">
									<xsl:for-each select="Evaluations/Evaluation">
										<TD>
										<xsl:value-of select="Value" />
									</TD>
									</xsl:for-each>
									<TD>
                    <xsl:element name="a">
                      <xsl:attribute name="href">
                        javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');





                      </xsl:attribute>
                      Click here to see configuration
                    </xsl:element>
									</TD>
								</TR>
							</xsl:for-each>	
						</TABLE>
					 </xsl:otherwise>
				 </xsl:choose>
				</TD>
				</TR>
					</TABLE>


</xsl:template>

  <xsl:template match="HiP-HOPS_Results/CoitGA">
    <TABLE id="ContentTable" cellSpacing="0">
      <TR>
        <TD>
          <xsl:choose>
            <xsl:when test="Population">
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/Objective">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          
javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');




                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:when>
            <xsl:otherwise>
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/*">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                            javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);


                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');





                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:otherwise>
          </xsl:choose>
        </TD>
      </TR>
    </TABLE>


  </xsl:template>


  <xsl:template match="HiP-HOPS_Results/SafetyAllocations">
    <TABLE id="ContentTable" cellSpacing="0">
      <TR>
        <TD>
          <xsl:choose>
            <xsl:when test="Population">
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/Objective">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
						javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);
                        
                          

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
				<xsl:choose>
				<xsl:when test="not($sortType='')">
				<xsl:comment>sortType is defined, use sort</xsl:comment>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.=$sortType]]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
				</xsl:when>
				<xsl:otherwise>
				<xsl:comment>sortType is undefined, do NOT use sort</xsl:comment>
				<xsl:for-each select="Population/Individual">
                  
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');



                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
				
				</xsl:otherwise>
				</xsl:choose>
			  </TABLE>
            </xsl:when>
            <xsl:otherwise>
              <TABLE id="CutSetTable" cellSpacing="0">
                <TR class="HeaderRow">
                  <xsl:for-each select="Objectives/*">
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                          javascript:
						var sortOrderParam = '<xsl:value-of select="$sortOrder" />';
						if (sortOrderParam == 'ascending') {
                          sortOrderParam = 'descending';
                        }
						else {
							sortOrderParam = 'ascending';
						}
						
                        optimisationTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="Name" />', sortOrderParam);

                        </xsl:attribute>
                        <xsl:value-of select="Name" />
                      </xsl:element>
                    </TD>
                  </xsl:for-each>
                  <TD>
                    Configuration
                  </TD>
                </TR>
                <xsl:for-each select="Population/Individual">
                  <xsl:sort select="Evaluations/Evaluation[Name[.='{$sortType}']]/Value" order="{$sortOrder}" data-type="number"/>
                  <TR class="DataRow">
                    <xsl:for-each select="Evaluations/Evaluation">
                      <TD>
                        <xsl:value-of select="Value" />
                      </TD>
                    </xsl:for-each>
                    <TD>
                      <xsl:element name="a">
                        <xsl:attribute name="href">
                 

javascript:
						  optimisationIndividualTransform('<xsl:value-of select="$XmlFileName" />','<xsl:value-of select="local-name(Encoding/*[1])" />', '<xsl:value-of select="IndividualID" />');
                          
                          optimisationIndividualSummaryTransform('<xsl:value-of select="$XmlFileName" />', '<xsl:value-of select="IndividualID" />');

                        </xsl:attribute>
                        Click here to see configuration
                      </xsl:element>
                    </TD>
                  </TR>
                </xsl:for-each>
              </TABLE>
            </xsl:otherwise>
          </xsl:choose>
        </TD>
      </TR>
    </TABLE>


  </xsl:template>
</xsl:stylesheet>