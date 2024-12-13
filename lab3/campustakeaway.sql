/*==============================================================*/
/* DBMS name:      MySQL 5.0                                    */
/* Created on:     2024/10/24 19:46:28                          */
/*==============================================================*/


drop table if exists cafe_admin;

drop table if exists cafeteria;

drop table if exists comment;

drop table if exists dish;

drop table if exists merchant;

drop table if exists normal_user;

drop table if exists orders;

drop table if exists orders_dish;

drop table if exists shangpu;

drop table if exists user;

drop table if exists user_role;

/*==============================================================*/
/* Table: cafe_admin                                            */
/*==============================================================*/
create table cafe_admin
(
   cafeadmin_id         int not null,
   user_id              int not null,
   cafeadmin_name       varchar(50) not null,
   primary key (cafeadmin_id)
);

/*==============================================================*/
/* Table: cafeteria                                             */
/*==============================================================*/
create table cafeteria
(
   cafeteria_id         int not null,
   cafeadmin_id         int,
   cafeteria_name       varchar(50),
   primary key (cafeteria_id)
);

/*==============================================================*/
/* Table: comment                                               */
/*==============================================================*/
create table comment
(
   comment_id           int not null,
   order_id             int not null,
   comment_text         text,
   comment_create_time  datetime,
   primary key (comment_id)
);

/*==============================================================*/
/* Table: dish                                                  */
/*==============================================================*/
create table dish
(
   dish_id              int not null,
   shangpu_id           int,
   price                numeric(8,0) not null,
   dish_text            text,
   dish_name            varchar(100) not null,
   dish_score           numeric(8,0),
   primary key (dish_id)
);

/*==============================================================*/
/* Table: merchant                                              */
/*==============================================================*/
create table merchant
(
   merchant_id          int not null,
   user_id              int not null,
   merchant_name        varchar(50) not null,
   merchant_text        text,
   primary key (merchant_id)
);

/*==============================================================*/
/* Table: normal_user                                           */
/*==============================================================*/
create table normal_user
(
   normal_user_id       int not null,
   user_id              int not null,
   normal_user_name     varchar(50),
   history_orders       int,
   vip_level            int,
   primary key (normal_user_id)
);

/*==============================================================*/
/* Table: orders                                                */
/*==============================================================*/
create table orders
(
   order_id             int not null,
   normal_user_id       int,
   merchant_id          int,
   order_status         varchar(20) not null,
   address              varchar(100) not null,
   create_time          datetime,
   primary key (order_id)
);

/*==============================================================*/
/* Table: orders_dish                                           */
/*==============================================================*/
create table orders_dish
(
   order_id             int not null,
   dish_id              int not null,
   primary key (order_id, dish_id)
);

/*==============================================================*/
/* Table: shangpu                                               */
/*==============================================================*/
create table shangpu
(
   shangpu_id           int not null,
   merchant_id          int,
   cafeteria_id         int not null,
   shangpu_name         varchar(50) not null,
   shangpu_text         text not null,
   shangpu_score        decimal,
   primary key (shangpu_id)
);

/*==============================================================*/
/* Table: user                                                  */
/*==============================================================*/
create table user
(
   user_id              int not null,
   role_id              int,
   password             varchar(50) not null,
   primary key (user_id)
);

/*==============================================================*/
/* Table: user_role                                             */
/*==============================================================*/
create table user_role
(
   role_id              int not null,
   role_name            varchar(20) not null,
   primary key (role_id)
);

alter table cafe_admin add constraint FK_use_admin foreign key (user_id)
      references user (user_id) on delete restrict on update restrict;

alter table cafeteria add constraint FK_admin_cafeteria foreign key (cafeadmin_id)
      references cafe_admin (cafeadmin_id) on delete restrict on update restrict;

alter table comment add constraint FK_orders_comment foreign key (order_id)
      references orders (order_id) on delete restrict on update restrict;

alter table dish add constraint FK_shangpu_dish foreign key (shangpu_id)
      references shangpu (shangpu_id) on delete restrict on update restrict;

alter table merchant add constraint FK_user_merchant foreign key (user_id)
      references user (user_id) on delete restrict on update restrict;

alter table normal_user add constraint FK_user_normal foreign key (user_id)
      references user (user_id) on delete restrict on update restrict;

alter table orders add constraint FK_merchant_order foreign key (merchant_id)
      references merchant (merchant_id) on delete restrict on update restrict;

alter table orders add constraint FK_normal_user_orders foreign key (normal_user_id)
      references normal_user (normal_user_id) on delete restrict on update restrict;

alter table orders_dish add constraint FK_orders_dish foreign key (order_id)
      references orders (order_id) on delete restrict on update restrict;

alter table orders_dish add constraint FK_orders_dish2 foreign key (dish_id)
      references dish (dish_id) on delete restrict on update restrict;

alter table shangpu add constraint FK_cafeteria_shangpu foreign key (cafeteria_id)
      references cafeteria (cafeteria_id) on delete restrict on update restrict;

alter table shangpu add constraint FK_merchant_shangpu foreign key (merchant_id)
      references merchant (merchant_id) on delete restrict on update restrict;

alter table user add constraint FK_user_userole foreign key (role_id)
      references user_role (role_id) on delete restrict on update restrict;

