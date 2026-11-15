<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.11.2" name="tiles" tilewidth="16" tileheight="16" tilecount="768" columns="16">
 <image source="../images/tiles.png" width="256" height="768"/>
 <!-- Explicit collision defaults for every non-empty tile used by world_map.json.
      The interior floor at global GID 275 is the only confirmed walkable tile. -->
 <tile id="0"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="6"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="7"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="14"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="22"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="23"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="29"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="31"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="37"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="46"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="62"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="257"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="259"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="266"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="273"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="275"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="289"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="290"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="291"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="296"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="297"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <!-- Authored device fixtures live on the sealed south border. The neighboring
      empty cells keep this region outside normal overworld movement. -->
 <tile id="526"><properties><property name="water" type="bool" value="true"/></properties></tile>
 <tile id="527"><properties><property name="walkable" type="bool" value="true"/><property name="water" type="bool" value="true"/></properties></tile>
 <tile id="528"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <!-- Pocket prop tiles. The box (544) stays a non-walkable container and
      carries the spare prop (bit 5) so WorldEngine::interact() picks it from
      the RAM property window; the plant (550) carries the plantable prop for
      the faced gather hook. Map words are tsx id + 1. -->
 <tile id="260"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="261"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="274"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="544"><properties><property name="walkable" type="bool" value="false"/><property name="spare" type="bool" value="true"/></properties></tile>
 <tile id="550"><properties><property name="walkable" type="bool" value="false"/><property name="plantable" type="bool" value="true"/></properties></tile>
 <tile id="599"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="600"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="601"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="602"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="603"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="604"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="605"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="606"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="607"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="608"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="609"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="610"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="611"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="612"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="613"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="614"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="615"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="616"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="617"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="618"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="619"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="620"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="621"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="622"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="623"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="624"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="625"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="626"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="627"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="628"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
 <tile id="629"><properties><property name="walkable" type="bool" value="true"/><property name="encounter" type="bool" value="true"/></properties></tile>
 <tile id="630"><properties><property name="walkable" type="bool" value="false"/></properties></tile>
</tileset>
