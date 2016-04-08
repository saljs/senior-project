//base platform
color("brown")
{
    cube([150, 250, 6.35]);
}

//right motor 
translate([0, 20, -20])
{
    cube([23, 74, 20]);
}
//right wheel
translate([-26, 40, -10])
{
    rotate([0, 90, 0])
    {
        color("black")
        {
            cylinder(d = 70, h = 26);
        }
    }
}
//left motor
translate([127, 20, -20])
{
    cube([23, 74, 20]);
}
//right wheel
translate([150, 40, -10])
{
    rotate([0, 90, 0])
    {
        color("black")
        {
            cylinder(d = 70, h = 26);
        }
    }
}
//batery
translate([43, 80, -23])
{
    color("black")
    {
        cube([64, 150, 23]);
    }
}