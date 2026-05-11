flowchart TD

    START["fs_open_submit(path)"]

    INIT["Set Initial Directory Cluster"]

    PARSE["Parse Directory Sector"]

    ENTRY{"Directory Entry Match?"}

    ENDMARK{"Entry == 0x00?"}

    DELETED{"Deleted / LFN?"}

    FINAL{"Final Path Component?"}

    DESCEND["Descend into Subdirectory"]

    OPENFILE["Populate FILE Structure"]

    NEXTSECTOR["Advance to Next Sector"]

    NEEDFAT{"End of Cluster?"}

    READFAT["Read FAT Entry"]

    NEXTCLUSTER{"Valid Next Cluster?"}

    DONE["FSOPEN_DONE"]

    NOTFOUND["FSOPEN_NOT_FOUND"]

    START --> INIT
    INIT --> PARSE

    PARSE --> ENDMARK

    ENDMARK -->|Yes| NOTFOUND
    ENDMARK -->|No| DELETED

    DELETED -->|Skip| PARSE
    DELETED -->|Valid Entry| ENTRY

    ENTRY -->|No Match| NEXTSECTOR

    ENTRY -->|Match| FINAL

    FINAL -->|Yes| OPENFILE
    FINAL -->|No| DESCEND

    DESCEND --> PARSE

    OPENFILE --> DONE

    NEXTSECTOR --> NEEDFAT

    NEEDFAT -->|No| PARSE

    NEEDFAT -->|Yes| READFAT

    READFAT --> NEXTCLUSTER

    NEXTCLUSTER -->|Invalid| NOTFOUND
    NEXTCLUSTER -->|Valid| PARSE
