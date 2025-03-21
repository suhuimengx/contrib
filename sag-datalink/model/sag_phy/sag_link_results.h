/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Magister Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Budiarto Herman <budiarto.herman@magister.fi>
 *
 */

#ifndef SATELLITE_LINK_RESULTS_H
#define SATELLITE_LINK_RESULTS_H

#include "sag_mutual_information.h"
#include "sag_enums.h"
#include "sag_lookup_table.h"

#include <ns3/string.h>
#include <ns3/object.h>
#include <ns3/ptr.h>
#include <map>


namespace ns3
{

/**
 * \ingroup satellite
 *
 * \brief Abstract class for storing link results.
 *
 * This class cannot be instantiated. Use the child classes instead.
 *
 * The child class is expected to load and handle multiple SatLookUpTable, and
 * provide query service on it.
 *
 * Usage example:
 *
 *     Ptr<SatLinkResultsDvbRcs2> linkDvbRcs2 = CreateObject<SatLinkResultsDvbRcs2> ();
 *     Ptr<SatLinkResultsDvbS2> linkDvbS2 = CreateObject<SatLinkResultsDvbRcs2> ();
 *     linkDvbRcs2->Initialize ();
 *     linkDvbS2->Initialize ();
 *
 */
class SatLinkResults : public Object
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResults();

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Initialize look up tables.
     *
     * Must be called before any SatLinkResults::GetBler can be invoked.
     *
     * Simply calls SatLinkResults::DoInitialize.
     */
    void Initialize();
    void SetBaseDir(std::string baseDir);

  protected:
    /**
     * \brief Initialize look up tables.
     *
     * Child classes must implement this function to initialize
     * m_table member variable. This is typically done by loading
     * pre-defined input files from the file system. In case of failure, the
     * function should throw an error by calling `NS_FATAL_ERROR`.
     */
    virtual void DoInitialize() = 0;

    /**
     * \brief The base path where the text
     *        files containing link results data can be found.
     */
    std::string m_inputPath;

    /**
     * \brief Indicates if SatLinkResults::Initialize has been called.
     */
    bool m_isInitialized;

  private:
    std::string m_baseDir;
};

/**
 * \ingroup satellite
 *
 * \brief Link results for DVB-RCS2.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * waveform id.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsRtn : public SatLinkResults
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsRtn();

    ~SatLinkResultsRtn()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Get a BLER value from link results.
     *
     * \param waveformId (which determines burst length and MODCOD)
     * \param ebNoDb the received Eb/No in dB
     * \return BLER value, which is a `double` ranging between [0..1]
     *
     * Must be run after SatLinkResults::Initialize is called.
     *
     */
    double GetBler(uint32_t waveformId, double ebNoDb) const;

    /**
     * \brief Get a Eb/No requirement for a given BLER target from link results.
     *
     * \param waveformId (which determines burst length and MODCOD)
     * \param blerTarget Target BLER for the system
     * \return Eb/No value
     *
     * Must be run after SatLinkResults::Initialize is called.
     *
     */
    double GetEbNoDb(uint32_t waveformId, double blerTarget) const;

  protected:
    /**
     * \brief Initialize by loading DVB-RCS2 look up tables.
     */
    virtual void DoInitialize() = 0;

    /**
     * \brief Map of satellite link result look up tables.
     * - key = uint32_t, i.e. waveform id
     * - value = Ptr<SatLookUpTable>, i.e. look-up table containing the link results
     */
    std::map<uint32_t, Ptr<SatLookUpTable>> m_table;
};

/**
 * \ingroup satellite
 *
 * \brief Link results for DVB-RCS2.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * waveform id.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsDvbRcs2 : public SatLinkResultsRtn
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsDvbRcs2();

    ~SatLinkResultsDvbRcs2()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

  protected:
    /**
     * \brief Initialize by loading DVB-RCS2 look up tables.
     */
    virtual void DoInitialize();
};

/**
 * \ingroup satellite
 *
 * \brief Link results for F-SIM.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * waveform id. Loads and maintains a Mutual Information table.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsFSim : public SatLinkResultsRtn
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsFSim();

    ~SatLinkResultsFSim()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the Normalized Symbol Information corresponding to a given SNIR
     * \param snirDb SNIR in logarithmic scale
     * \return Normalized  Symbol Information
     */
    inline double GetNormalizedSymbolInformation(double snirDb) const
    {
        return m_mutualInformationTable->GetNormalizedSymbolInformation(snirDb);
    }

    /**
     * \brief Get the SNIR in dB for a given Normalized Symbol Information target
     * \param Normalizd Symbol Information target (0-1)
     * \return Snir target in dB
     */
    inline double GetSnirDb(double symbolInformationTarget) const
    {
        return m_mutualInformationTable->GetSnirDb(symbolInformationTarget);
    }

    /**
     * \brief Get the Mutual Information Table
     */
    inline Ptr<SatMutualInformationTable> GetMutualInformationTable() const
    {
        return m_mutualInformationTable;
    }

  protected:
    /**
     * \brief Initialize by loading F-SIM look up tables.
     */
    void DoInitialize();

  private:
    /**
     * \brief Mutual information table.
     */
    Ptr<SatMutualInformationTable> m_mutualInformationTable;
};

/**
 * \ingroup satellite
 *
 * \brief Link results for F-SIM.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * waveform id. Loads and maintains a Mutual Information table.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsLora : public SatLinkResultsRtn
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsLora();

    ~SatLinkResultsLora()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Get the Normalized Symbol Information corresponding to a given SNIR
     * \param snirDb SNIR in logarithmic scale
     * \return Normalized  Symbol Information
     */
    inline double GetNormalizedSymbolInformation(double snirDb) const
    {
        return m_mutualInformationTable->GetNormalizedSymbolInformation(snirDb);
    }

    /**
     * \brief Get the SNIR in dB for a given Normalized Symbol Information target
     * \param Normalizd Symbol Information target (0-1)
     * \return Snir target in dB
     */
    inline double GetSnirDb(double symbolInformationTarget) const
    {
        return m_mutualInformationTable->GetSnirDb(symbolInformationTarget);
    }

    /**
     * \brief Get the Mutual Information Table
     */
    inline Ptr<SatMutualInformationTable> GetMutualInformationTable() const
    {
        return m_mutualInformationTable;
    }

  protected:
    /**
     * \brief Initialize by loading F-SIM look up tables.
     */
    void DoInitialize();

  private:
    /**
     * \brief Mutual information table.
     */
    Ptr<SatMutualInformationTable> m_mutualInformationTable;
};

/**
 * \ingroup satellite
 *
 * \brief Link results for forward link.
 *
 * The instance will be SatLinkResultsDvbS2 or SatLinkResultsDvbS2X.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * modulation and coding scheme.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsFwd : public SatLinkResults
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsFwd();

    ~SatLinkResultsFwd()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Get a BLER value from link results.
     *
     * \param modcod Modulation and coding scheme
     * \param frameType BB frame type (short, long)
     * \param esNoDb the received Es/No in dB
     * \return BLER value, which is a `double` ranging between [0..1]
     *
     * Must be run after SatLinkResults::Initialize is called.
     *
     */
    double GetBler(SatEnums::SatModcod_t modcod,
                   SatEnums::SatBbFrameType_t frameType,
                   double esNoDb) const;

    /**
     * \brief Get a Es/No requirement for a given BLER target from link results.
     *
     * \param modcod Modulation and coding scheme
     * \param frameType BB frame type (short, long)
     * \param blerTarget Target BLER for the system
     * \return Es/No value
     *
     * Must be run after SatLinkResults::Initialize is called.
     *
     */
    double GetEsNoDb(SatEnums::SatModcod_t modcod,
                     SatEnums::SatBbFrameType_t frameType,
                     double blerTarget) const;

  protected:
    /**
     * \brief Initialize look up tables.
     *
     * Child classes must implement this function to initialize
     * m_table member variable. This is typically done by loading
     * pre-defined input files from the file system. In case of failure, the
     * function should throw an error by calling `NS_FATAL_ERROR`.
     */
    virtual void DoInitialize() = 0;

    /**
     * \brief Map of satellite link result look up tables.
     * - key = SatModcod_e, i.e. modulation and coding scheme
     * - value = Ptr<SatLookUpTable>, i.e. look-up table containing the link results
     */
    std::map<SatEnums::SatModcod_t, Ptr<SatLookUpTable>> m_table;

    double m_shortFrameOffsetInDb;
};

/**
 * \ingroup satellite
 *
 * \brief Link results for DVB-S2.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * modulation and coding scheme.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsDvbS2 : public SatLinkResultsFwd
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsDvbS2();

    ~SatLinkResultsDvbS2()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

  private:
    /**
     * \brief Initialize by loading DVB-S2 look up tables.
     */
    void DoInitialize();
};

/**
 * \ingroup satellite
 *
 * \brief Link results for DVB-S2X.
 *
 * Loads and maintains multiple SatLookUpTable. Provides query service based on
 * modulation and coding scheme.
 *
 * See usage examples in the parent class documentation (SatLinkResults).
 */
class SatLinkResultsDvbS2X : public SatLinkResultsFwd
{
  public:
    /**
     * Default constructor.
     */
    SatLinkResultsDvbS2X();

    ~SatLinkResultsDvbS2X()
    {
    }

    /**
     * \brief Get the type ID
     * \return the object TypeId
     */
    static TypeId GetTypeId();

  private:
    /**
     * \brief Initialize by loading DVB-S2 look up tables.
     */
    void DoInitialize();
};

} // end of namespace ns3

#endif /* SATELLITE_LINK_RESULTS_H */
